
#include <cassert>
#include <google/protobuf/message.h>

#include "utils/timeutils.h"

#include "base.h"
#include "config.h"

#include "dbthreads.h"
#include "agentservice.h"

#include "proxyserver.h"

CProxyServer::CProxyServer()
    : m_DBThreads( NULL ),
      m_AgentService( NULL )
{}

CProxyServer::~CProxyServer()
{}

bool CProxyServer::onStart()
{
    m_AgentService = new CAgentService(
            CDBProxyConfig::getInstance().getServiceThreads(),
            CDBProxyConfig::getInstance().getMaxClients() );
    assert( m_AgentService != NULL && "create CAgentService failed" );

    // 初始化AgentService
    if ( !m_AgentService->listen(
                CDBProxyConfig::getInstance().getBindHost().c_str(),
                CDBProxyConfig::getInstance().getListenPort() ) )
    {
        LOG_FATAL( "CAgentService(%d, %d) listen(%s::%d) failed .\n",
                CDBProxyConfig::getInstance().getServiceThreads(),
                CDBProxyConfig::getInstance().getMaxClients(),
                CDBProxyConfig::getInstance().getBindHost().c_str(),
                CDBProxyConfig::getInstance().getListenPort() );
        return false;
    }
    LOG_INFO( "CAgentService(%d, %d) listen(%s::%d) succeed .\n",
            CDBProxyConfig::getInstance().getServiceThreads(),
            CDBProxyConfig::getInstance().getMaxClients(),
            CDBProxyConfig::getInstance().getBindHost().c_str(),
            CDBProxyConfig::getInstance().getListenPort() );

    m_DBThreads = new DBThreads( CDBProxyConfig::getInstance().getDBThreads() );
    assert( m_DBThreads != NULL && "create DBThreads failed" );

    // 初始化DBThreads
    if ( !m_DBThreads->initialize() )
    {
        LOG_FATAL( "DBThreads(%d) initialize failed .\n", CDBProxyConfig::getInstance().getDBThreads() );
        return false;
    }
    LOG_INFO( "DBThreads(%d) initialize succeed .\n", CDBProxyConfig::getInstance().getDBThreads() );

    return true;
}

void CProxyServer::onExecute()
{
    if ( !m_DBThreads->check() )
    {
        this->stop( false );
        return;
    }

    Utils::TimeUtils::sleep( 100 );
}

void CProxyServer::onStop()
{
    if ( m_AgentService != NULL )
    {
        m_AgentService->stop();
        delete m_AgentService;
        m_AgentService = NULL;
    }

    if ( m_DBThreads != NULL )
    {
        m_DBThreads->finalize();
        delete m_DBThreads;
        m_DBThreads = NULL;
    }

    google::protobuf::ShutdownProtobufLibrary();
}
