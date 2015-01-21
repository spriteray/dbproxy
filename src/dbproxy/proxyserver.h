
#ifndef __SRC_DBPROXY_PROXYSERVER_H__
#define __SRC_DBPROXY_PROXYSERVER_H__

#include "utils/thread.h"
#include "utils/singleton.h"

class DBThreads;
class CAgentService;

class CProxyServer : public Utils::IThread, public Singleton<CProxyServer>
{
public :
    CProxyServer();
    virtual ~CProxyServer();

    virtual bool onStart();
    virtual void onExecute();
    virtual void onStop();

public :
    DBThreads * getDBThreads() const { return m_DBThreads; }
    CAgentService * getAgentService() const { return m_AgentService; }

private :
    DBThreads *     m_DBThreads;
    CAgentService * m_AgentService;
};

#endif
