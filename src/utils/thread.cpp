
#include <vector>

#include <errno.h>
#include <signal.h>

#include "time.h"
#include "thread.h"

namespace Utils
{

IThread::IThread()
{
    m_Status    = eReady;
    m_IsDetach  = false;
    m_StackSize = 0;
    m_ThreadID  = 0;
    pthread_cond_init( &m_CtrlCond, NULL );
    pthread_mutex_init( &m_CtrlLock, NULL );
}

IThread::~IThread()
{
    m_Status    = eReady;
    m_IsDetach  = false;
    m_StackSize = 0;
    m_ThreadID  = 0;
    pthread_cond_destroy( &m_CtrlCond );
    pthread_mutex_destroy( &m_CtrlLock );
}

pthread_t IThread::id() const
{
    return m_ThreadID;
}

bool IThread::isRunning() const
{
    return ( m_Status == eRunning );
}

void IThread::setDetach()
{
    m_IsDetach = true;
}

void IThread::setStackSize( uint32_t size )
{
    m_StackSize = size;
}

bool IThread::start()
{
    int32_t rc = 0;
    pthread_attr_t attr;

    pthread_attr_init( &attr );

    if ( m_StackSize > 0 )
    {
        pthread_attr_setstacksize( &attr, m_StackSize );
    }
    if ( m_IsDetach )
    {
        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    }

    m_Status = eRunning;
    rc = pthread_create( &m_ThreadID, &attr, threadfunc, this );
    pthread_attr_destroy( &attr );

    return ( rc == 0 );
}

void IThread::stop( bool iswait )
{
    pthread_mutex_lock( &m_CtrlLock );

    if ( m_Status != eStoped )
    {
        m_Status = eStoping;
    }

    while ( iswait && m_Status != eStoped )
    {
        pthread_cond_wait( &m_CtrlCond, &m_CtrlLock );
    }

    pthread_mutex_unlock( &m_CtrlLock );
}

void IThread::wait()
{
    pthread_mutex_lock( &m_CtrlLock );

    while ( m_Status != eStoped )
    {
        pthread_cond_wait( &m_CtrlCond, &m_CtrlLock );
    }

    pthread_mutex_unlock( &m_CtrlLock );
}

void IThread::notify()
{
    pthread_mutex_lock( &m_CtrlLock );

    m_Status = eStoped;
    pthread_cond_signal( &m_CtrlCond );

    pthread_mutex_unlock( &m_CtrlLock );
}

bool IThread::check( pthread_t id )
{
    int32_t rc = pthread_kill( id, 0 );
    return ( rc != ESRCH && rc != EINVAL );
}

void * IThread::threadfunc( void * arg )
{
    IThread * thread = (IThread *)arg;

    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);

    // 线程开始
    if ( thread->onStart() )
    {
        while ( thread->isRunning() )
        {
            thread->onExecute();
        }
    }

    // 线程停止了
    thread->onStop();

    // 通知调用者, 线程已经停止了
    thread->notify();

    return NULL;
}

// ----------------------------------------------------------------------------

IWorkThread::IWorkThread()
{
    pthread_mutex_init( &m_Lock, NULL );
}

IWorkThread::~IWorkThread()
{
    pthread_mutex_destroy( &m_Lock );
}

void IWorkThread::onExecute()
{
    std::deque<Task> tasklist;

    // TODO: 是否需要一个开始的回调

    pthread_mutex_lock( &m_Lock );
    std::swap( m_TaskQueue, tasklist );
    pthread_mutex_unlock( &m_Lock );

    // 回调逻辑层
    std::deque<Task>::iterator it;
    for ( it = tasklist.begin();it != tasklist.end(); ++it )
    {
        this->onTask( it->type, it->task );
    }

    // 任务线程空闲
    this->onIdle();
}

void IWorkThread::cleanup()
{
    pthread_mutex_lock( &m_Lock );

    while ( !m_TaskQueue.empty() )
    {
        Task task = m_TaskQueue.front();
        m_TaskQueue.pop_front();
        this->onTask( task.type, task.task );
    }
    m_TaskQueue.clear();

    pthread_mutex_unlock( &m_Lock );
}

bool IWorkThread::post( int32_t type, void * task )
{
    bool rc = false;

    pthread_mutex_lock( &m_Lock );

    if ( isRunning() )
    {
        Task tmp = { type, task };

        rc = true;
        m_TaskQueue.push_back( tmp );
    }

    pthread_mutex_unlock( &m_Lock );

    return rc;
}

// ----------------------------------------------------------------------------

#if 0

DBThread::DBThread( const char * host, uint16_t port,
        const char * username, const char * password, const char * encodings, const char * dbname )
    : m_Host( host ),
      m_Port( port ),
      m_Username( username ),
      m_Password( password ),
      m_Encoding( encoding ),
      m_DBname( dbname )
{
    pthread_mutex_init( &m_Lock, NULL );
    m_TaskQueue.clear();
}

DBThread::~DBThread()
{
    pthread_mutex_lock( &m_Lock );
    m_ThreadQueue.clear();
    pthread_mutex_unlock( &m_Lock );
    pthread_mutex_destroy( &m_Lock );
}

bool DBThread::onStart()
{
    // 初始化DB句柄
    m_DBHandler = mysql_init( NULL );

    int8_t reConnect = 1;
    mysql_options( mMysqlhandler, MYSQL_OPT_RECONNECT, (const char *)&reConnect );
    mysql_options( mMysqlhandler, MYSQL_SET_CHARSET_NAME, m_Encoding.c_str() );

    if ( mysql_real_connect( m_DBHandler,
                m_Host.c_str(),
                m_Username.c_str(),
                m_Password.c_str(),
                m_DBname.c_str(),
                m_Port, NULL, 0 ) == NULL )
    {
        mysql_close( m_DBHandler );
        m_DBHandler = NULL;
        return false;
    }
}

void DBThread::onExecute()
{
    std::vector<IDBRequest*> tasklist;
    tasklist.reserve(eMaxFetchTaskCount);

    pthread_mutex_lock( &m_Lock );

    if ( m_TaskQueue.empty() )
    {
        pthread_mutex_unlock( &m_Lock );

        // 任务线程空闲
        TimeUtils::sleep(eMaxWaitForTaskInterval);

        return;
    }

    while ( !m_TaskQueue.empty()
        && tasklist.size() < eMaxFetchTaskCount )
    {
        IDBRequest * record = m_TaskQueue.front();
        m_TaskQueue.pop_front();

        tasklist.push_back( record );
    }

    pthread_mutex_unlock( &m_Lock );

    // 回调逻辑层
    std::vector<IDBRequest *>::iterator it;
    for ( it = tasklist.begin();it != tasklist.end(); ++it )
    {
        IDBRequest * request = (IDBRequest *)it;

        doQuery( record );
        request->destroy();
    }

    return;
}

void DBThread::onStop()
{
    pthread_mutex_lock( &m_Lock );

    while ( !m_TaskQueue.empty() )
    {
        IDBRequest * request = m_TaskQueue.front();
        m_TaskQueue.pop_front();

        doQuery( request );
        request->destroy();
    }

    pthread_mutex_unlock( &m_Lock );

    // 销毁mysql句柄
    mysql_close( m_DBHandler );
}

void DBThread::doQuery( IDBRequest * request )
{
    std::string sqlcmd;

    if ( !request->serialize(sqlcmd) )
    {
        return;
    }

    // 查询数据库
    int32_t rc = mysql_real_query( m_DBHandler, sqlcmd.c_str(), sqlcmd.length() );

    // 返回结果
    if ( request->isResult() )
    {
        MYSQL_RES * result = NULL;

        if ( rc == 0 )
        {
            result = mysql_store_result( m_DBHandler );
        }

        // 返回结果
        request->onResult( result );

        if ( result != NULL )
        {
            mysql_free_result( result );
        }
    }

    return;
}

#endif

}

#if 0

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int8_t main_flag;

void signal_handle( int32_t signo )
{
    main_flag = 0;
}

class IWorkThread1 : public Utils::IWorkThread
{

public :

    IWorkThread1() {};
    ~IWorkThread1() {};

public :

    void onTask( int32_t type, void * arg )
    {
        printf("Task:%d \n", type);
    }

};


int main()
{
    int32_t type = 1;
    IWorkThread1 thread;

    signal( SIGINT, signal_handle );

    thread.start();

    main_flag = 1;
    while( main_flag )
    {
        thread.post( type, NULL );
        ++type;
    }

    thread.stop();

    return 0;
}

#endif
