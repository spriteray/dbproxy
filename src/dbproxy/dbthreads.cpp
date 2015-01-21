
#include "utils/utility.h"
#include "utils/timeutils.h"

#include "base.h"
#include "config.h"

#include "message/proxy.pb.h"

#include "dbengine.h"
#include "agentservice.h"

#include "dbthreads.h"

DBThread::DBThread( uint8_t index )
    : m_Index( index ),
      m_Engine( NULL ),
      m_KeepaliveTime( 0ULL )
{
    pthread_mutex_init( &m_Lock, NULL );
}

DBThread::~DBThread()
{
    pthread_mutex_destroy( &m_Lock );
}

bool DBThread::onStart()
{
    // 初始化DBEngine
    m_Engine = new DBEngine(
            CDBProxyConfig::getInstance().getDBHost(),
            CDBProxyConfig::getInstance().getDBPort() );
    if ( m_Engine == NULL )
    {
        // TODO: 日志
        return false;
    }

    // 设置DBEngine参数
    m_Engine->selectdb( CDBProxyConfig::getInstance().getDatabase() );
    m_Engine->setCharsets( CDBProxyConfig::getInstance().getDBCharsets() );
    m_Engine->setToken(
            CDBProxyConfig::getInstance().getDBUsername(),
            CDBProxyConfig::getInstance().getDBPassword() );

    if ( !m_Engine->initialize() )
    {
        LOG_FATAL( "DBThread(Index:%d, Database:'%s') initialize failed .\n",
                m_Index,
                CDBProxyConfig::getInstance().getDatabase().c_str() );
        return false;
    }

    // 初始化Keepalive时间戳
    m_KeepaliveTime = Utils::TimeUtils::now();

    LOG_INFO( "DBThread(Index:%d, Database:%s) initialize succeed .\n",
            m_Index, CDBProxyConfig::getInstance().getDatabase().c_str() );

    return true;
}

void DBThread::onExecute()
{
    int64_t now = Utils::TimeUtils::now();

    // 查询数据库
    this->process();

    // MYSQL保活功能
    if ( now - m_KeepaliveTime >= eEngine_KeepaliveSeconds  )
    {
        m_KeepaliveTime = now;
        m_Engine->keepalive();
    }

    // 防止空转
    int64_t used_msecs = Utils::TimeUtils::now() - now;
    if ( used_msecs >= 0 && used_msecs < eProxy_EachFrameSeconds )
    {
        Utils::TimeUtils::sleep( eProxy_EachFrameSeconds-used_msecs );
    }
}

void DBThread::onStop()
{
    LOG_INFO( "DBThread(Index:%d, Database:'%s') stop ...\n",
            m_Index, CDBProxyConfig::getInstance().getDatabase().c_str() );

    this->process();

    if ( m_Engine != NULL )
    {
        m_Engine->finalize();
        delete m_Engine;
        m_Engine = NULL;
    }
}

bool DBThread::push2Database( SQLCmd cmd,
        sid_t sid, uint32_t transid, const std::string & sqlcmd )
{
    if ( !this->isRunning() )
    {
        LOG_ERROR( "DBThread::push2Database(SID:%lu, TransID:%d, SQLCMD:'%s') : this DBThread(%d) is STOP .\n",
                sid, transid, sqlcmd.c_str(), m_Index );
        return false;
    }

    pthread_mutex_lock( &m_Lock );
    m_Queue.push_back( DBCommand( cmd, sid, transid, sqlcmd ) );
    pthread_mutex_unlock( &m_Lock );

    return true;
}

void DBThread::process()
{
    std::deque<DBCommand> taskqueue;
    std::deque<DBCommand>::iterator it;

    // 交换任务队列
    pthread_mutex_lock( &m_Lock );
    std::swap( m_Queue, taskqueue );
    pthread_mutex_unlock( &m_Lock );

    for ( it = taskqueue.begin(); it != taskqueue.end(); ++it )
    {
        switch ( it->cmd )
        {
            case eSQLCmd_Insert :
                this->insert( it->sid, it->transid, it->sqlcmd );
                break;

            case eSQLCmd_Query :
                this->query( it->sid, it->transid, it->sqlcmd );
                break;

            case eSQLCmd_Update :
                this->update( it->sid, it->transid, it->sqlcmd );
                break;

            case eSQLCmd_Remove :
                this->remove( it->sid, it->transid, it->sqlcmd );
                break;

            case eSQLCmd_None :
            default :
                break;
        }
    }
}

void DBThread::insert( sid_t sid, uint32_t transid, const std::string & sqlcmd )
{
    uint64_t insertid = 0;

    bool rc = m_Engine->insert( sqlcmd, insertid );
    if ( !rc )
    {
        return;
    }

    std::string id;
    Utils::Utility::snprintf( id, 32, "%lu", insertid );

    // 返回结果给dbclient
    msg::proxy::MessageAutoIncrement msg;
    msg.set_insertid( id );
    g_AgentService->send( sid, transid, msg::proxy::eMessage_AutoIncrease, &msg );
}

void DBThread::query( sid_t sid, uint32_t transid, const std::string & sqlcmd )
{
    Results results;

    bool rc = m_Engine->query( sqlcmd, results );
    if ( !rc )
    {
        return;
    }

    // 返回结果给dbclient
    msg::proxy::MessageResultSets msg;
    for ( size_t i = 0; i < results.size(); ++i )
    {
        msg::proxy::MessageResultSets::Result * result = msg.add_results();
        if ( result )
        {
            for ( size_t j = 0; j < results[i].size(); ++j )
            {
                result->add_field( results[i][j] );
            }
        }
    }
    g_AgentService->send( sid, transid, msg::proxy::eMessage_Results, &msg );
}

void DBThread::update( sid_t sid, uint32_t transid, const std::string & sqlcmd )
{
    uint32_t naffected = 0;
    m_Engine->update( sqlcmd, naffected );
}

void DBThread::remove( sid_t sid, uint32_t transid, const std::string & sqlcmd )
{
    uint32_t naffected = 0;
    m_Engine->remove( sqlcmd, naffected );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

DBThreads::DBThreads( uint8_t nthreads )
    : m_Number( nthreads )
{}

DBThreads::~DBThreads()
{}

bool DBThreads::initialize()
{
    for ( uint8_t i = 0; i < m_Number; ++i )
    {
        DBThread * th = new DBThread( i );
        if ( th == NULL )
        {
            break;
        }

        // 设置线程属性
        th->setDetach();

        // 线程开始
        if ( !th->start() )
        {
            break;
        }

        m_Threads.push_back( th );
    }

    if ( m_Threads.size() != m_Number )
    {
        this->finalize();
        return false;
    }

    return true;
}

void DBThreads::finalize()
{
    for ( size_t i = 0; i < m_Threads.size(); ++i )
    {
        DBThread * th = m_Threads[ i ];

        if ( th != NULL )
        {
            th->stop();
            delete th;
        }
    }

    m_Threads.clear();
}

bool DBThreads::check()
{
    for ( size_t i = 0; i < m_Threads.size(); ++i )
    {
        if ( !m_Threads[i]->isRunning() )
        {
            return false;
        }
    }

    return true;
}

bool DBThreads::post( uint8_t index, SQLCmd cmd,
        sid_t sid, uint32_t transid, const std::string & sqlcmd )
{
    if ( index >= m_Threads.size() )
    {
        LOG_ERROR( "DBThreads::post(SID:%lu, TransID:%d, SQLCMD:'%s') : this DBThread's Index(%d) is Invalid .\n",
                sid, transid, sqlcmd.c_str(), index );
        return false;
    }

    return m_Threads[ index ]->push2Database( cmd, sid, transid, sqlcmd );
}
