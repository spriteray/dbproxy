
#ifndef __SRC_DBPROXY_AGENTSERVICE_H__
#define __SRC_DBPROXY_AGENTSERVICE_H__

#include "define.h"
#include "message.h"

#include "io/io.h"

#include "proxyserver.h"

class CDBClientSession : public IIOSession
{
public :
    CDBClientSession( const char * host, uint16_t port );
    virtual ~CDBClientSession();

    virtual int32_t onStart();
    virtual int32_t onProcess( const char * buffer, uint32_t nbytes );
    virtual int32_t onTimeout();
    virtual int32_t onError( int32_t result );
    virtual void    onShutdown( int32_t way );

private :
    void onMessage( DBHead * head, const char * buf );

private :
    std::string     m_Host;
    uint16_t        m_Port;
};

class CAgentService : public IIOService
{
public :
    CAgentService( uint8_t nthreads, uint32_t nclients );
    virtual ~CAgentService();

    virtual void * getLocalData( uint8_t index );
    virtual IIOSession * onAccept( sid_t id, const char * host, uint16_t port );

public :
    int32_t send( sid_t sid,
            uint32_t transid, uint16_t cmd, DBMessage * msg );

private :
    uint8_t         m_ThreadsCount;
};

#define g_AgentService      CProxyServer::getInstance().getAgentService()

#endif
