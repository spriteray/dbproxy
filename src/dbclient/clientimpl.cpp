
#include <cassert>
#include <google/protobuf/message.h>

#include "utils/timeutils.h"
#include "utils/transaction.h"

#include "timetick.h"
#include "transaction.h"
#include "agentclient.h"

#include "message/proxy.pb.h"

#include "clientimpl.h"

ClientImpl::ClientImpl()
    : m_Timeout( 0 ),
      m_Persicion( 0 ),
      m_ProxyPort( 0 ),
      m_ScheduleTime( 0ULL ),
      m_Client( NULL ),
      m_TimeTick( NULL ),
      m_TransactionManager( NULL )
{}

ClientImpl::~ClientImpl()
{
    google::protobuf::ShutdownProtobufLibrary();
}

void ClientImpl::schedule()
{
    // 处理结果队列
    m_Results.process( m_TransactionManager );

    // 定时器调度
    uint64_t now = Utils::TimeUtils::now();

    // 第一次调度
    if ( m_ScheduleTime == 0 )
    {
        m_ScheduleTime = now;
        return;
    }

    // 下次调度时间到了
    if ( m_ScheduleTime + m_Persicion <= now )
    {
        m_ScheduleTime = now;
        m_TimeTick->run();
    }
}

void ClientImpl::update( uint8_t index, const std::string & sqlcmd )
{
    msg::proxy::MessageUpdateSqlCmd msg;
    msg.set_index( index );
    msg.set_sqlcmd( sqlcmd );

    m_Client->send( msg::proxy::eMessage_Update, 0, &msg );
}

void ClientImpl::remove( uint8_t index, const std::string & sqlcmd )
{
    msg::proxy::MessageRemoveSqlCmd msg;
    msg.set_index( index );
    msg.set_sqlcmd( sqlcmd );

    m_Client->send( msg::proxy::eMessage_Remove, 0, &msg );
}

void ClientImpl::insert( uint8_t index, const std::string & sqlcmd, dbproxy::IDBResult * result, uint32_t timeout )
{
    uint32_t transid = 0;

    msg::proxy::MessageInsertSqlCmd msg;
    msg.set_index( index );
    msg.set_sqlcmd( sqlcmd );

    if ( result != NULL && timeout > 0 )
    {
        // 创建事务
        DBAutoIncrementTrans * transaction = new DBAutoIncrementTrans;
        assert( transaction != NULL && "create DBAutoIncrementTrans failed" );
        transaction->setResult( result );
        m_TransactionManager->append( transaction, timeout );

        // 获取新创建的事务ID
        transid = transaction->getTransID();
    }

    m_Client->send( msg::proxy::eMessage_Insert, transid, &msg );
}

void ClientImpl::query( uint8_t index, bool isbatch, const std::string & sqlcmd, dbproxy::IDBResult * result, uint32_t timeout )
{
    assert( result != NULL && timeout > 0 );

    msg::proxy::MessageQuerySqlCmd msg;
    msg.set_index( index );
    msg.set_batch( isbatch );
    msg.set_sqlcmd( sqlcmd );

    // 创建事务
    DBResultTrans * transaction = new DBResultTrans;
    assert( transaction != NULL && "create DBResultTrans failed" );
    transaction->setResult( result );
    m_TransactionManager->append( transaction, timeout );

    m_Client->send( msg::proxy::eMessage_Query, transaction->getTransID(), &msg );
}

////////////////////////////////////////////////////////////////////////////////

uint8_t ClientImpl::status() const
{
    return m_Client->getStatus();
}

bool ClientImpl::initialize()
{
    // 时间服务
    m_TimeTick = new TimeTick( m_Persicion );
    assert( m_TimeTick != NULL && "create TimeTick failed" );
    if ( !m_TimeTick->start() )
    {
        return false;
    }

    // 事务管理器
    m_TransactionManager = new Utils::TransactionManager( m_TimeTick->getTimer() );
    assert( m_TransactionManager != NULL && "create Utils::TransactionManager failed" );

    // 网络线程
    m_Client = new CAgentClient( &m_Results, m_Timeout );
    assert( m_Client != NULL && "create CAgentClient failed" );

    return m_Client->connect( m_ProxyHost.c_str(), m_ProxyPort, m_Timeout );
}

void ClientImpl::finalize()
{
    m_Results.process( m_TransactionManager );

    if ( m_Client != NULL )
    {
        m_Client->stop();
        delete m_Client;
        m_Client = NULL;
    }

    if ( m_TransactionManager != NULL )
    {
        delete m_TransactionManager;
        m_TransactionManager = NULL;
    }

    if ( m_TimeTick != NULL )
    {
        m_TimeTick->stop();
        delete m_TimeTick;
        m_TimeTick = NULL;
    }
}

void ClientImpl::setTimeout( uint32_t timeout )
{
    m_Timeout = timeout;
}

void ClientImpl::setPercision( uint32_t percision )
{
    m_Persicion = percision;
}

void ClientImpl::setEndpoint( const char * host, uint16_t port )
{
    m_ProxyHost = host;
    m_ProxyPort = port;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

dbproxy::IDBClient * dbproxy::IDBClient::connect(
            const char * host, uint16_t port,
            uint32_t timeout, uint32_t percition )
{
    // 初始化数据
    ClientImpl * client = new ClientImpl;
    assert( client != NULL && "create ClientImpl failed" );
    client->setTimeout( timeout );
    client->setPercision( percition );
    client->setEndpoint( host, port );

    // 连接
    if ( !client->initialize() )
    {
        client->finalize();
        delete client;
        return NULL;
    }

    // 阻塞状态连接服务器
    while ( client->status() != eConnected )
    {
        Utils::TimeUtils::sleep( 10 );
    }

    return client;
}
