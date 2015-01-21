
#ifndef __SRC_DBPROXY_DBTHREADS_H__
#define __SRC_DBPROXY_DBTHREADS_H__

#include <deque>
#include <vector>
#include <string>

#include "define.h"
#include "io/io.h"
#include "utils/thread.h"

#include "proxyserver.h"

class DBEngine;

class DBThread : public Utils::IThread
{
public :
    DBThread( uint8_t index );
    virtual ~DBThread();

    virtual bool onStart();
    virtual void onExecute();
    virtual void onStop();

public :
    bool push2Database( SQLCmd cmd, sid_t sid,
            uint32_t transid, const std::string & sqlcmd );

private :
    struct DBCommand
    {
        SQLCmd          cmd;
        sid_t           sid;
        uint32_t        transid;
        std::string     sqlcmd;

        DBCommand( SQLCmd cmd, sid_t sid,
                uint32_t transid, const std::string & sqlcmd )
        {
            this->cmd = cmd;
            this->sid = sid;
            this->transid = transid;
            this->sqlcmd = sqlcmd;
        }
    };

    enum
    {
        eEngine_KeepaliveSeconds        = 60*1000,  // 1分钟Keepalive一次
        eProxy_EachFrameSeconds         = 20,       // 20ms一帧
    };

    void process();

    void insert( sid_t sid, uint32_t transid, const std::string & sqlcmd );
    void query( sid_t sid, uint32_t transid, const std::string & sqlcmd );
    void update( sid_t sid, uint32_t transid, const std::string & sqlcmd );
    void remove( sid_t sid, uint32_t transid, const std::string & sqlcmd );

private :
    uint8_t                 m_Index;
    pthread_mutex_t         m_Lock;
    std::deque<DBCommand>   m_Queue;
    DBEngine *              m_Engine;
    int64_t                 m_KeepaliveTime;
};

class DBThreads
{
public :
    DBThreads( uint8_t nthreads );
    ~DBThreads();

public :
    bool initialize();
    void finalize();

    // 检查DBThreads
    bool check();

    bool post( uint8_t index, SQLCmd cmd,
            sid_t sid, uint32_t transid, const std::string & sqlcmd );

private :
    uint8_t                         m_Number;
    std::vector<DBThread *>         m_Threads;
};

#define g_DBThreads     CProxyServer::getInstance().getDBThreads()

#endif
