
#include <cassert>

#include "base.h"
#include "utils/file.h"

#include "config.h"

CDBProxyConfig::CDBProxyConfig()
{}

CDBProxyConfig::~CDBProxyConfig()
{}

bool CDBProxyConfig::load( const char * path )
{
    bool rc = false;
    Utils::ConfigFile doc( path );

    rc = doc.open();
    assert( rc && "CDBProxyConfig::load() failed" );

    // Global
    doc.get( "Global", "loglevel", m_LogLevel );
    doc.get( "Global", "dbthreads", m_DBThreads );
    doc.get( "Global", "logfilesize", m_LogFilesize );

    // Service
    doc.get( "Service", "host", m_BindHost );
    doc.get( "Service", "port", m_ListenPort );
    doc.get( "Service", "threads", m_ServiceThreads );
    doc.get( "Service", "maxclients", m_MaxClients );
    doc.get( "Service", "timeoutseconds", m_Timeoutseconds );

    // Database
    doc.get( "Database", "host", m_DBHost );
    doc.get( "Database", "port", m_DBPort );
    doc.get( "Database", "username", m_DBUsername );
    doc.get( "Database", "password", m_DBPassword );
    doc.get( "Database", "charsets", m_DBCharsets );
    doc.get( "Database", "database", m_Database );

    doc.close();
    LOG_TRACE( "CDBProxyConfig::load('%s') succeed .\n", path );

    return true;
}

bool CDBProxyConfig::reload( const char * path )
{
    // TODO:
    return true;
}

void CDBProxyConfig::unload()
{
    m_LogLevel      = 0;
    m_DBThreads     = 0;
    m_LogFilesize   = 0ULL;
    m_BindHost      = "";
    m_ListenPort    = 0;
    m_MaxClients    = 0;
    m_ServiceThreads= 0;
    m_Timeoutseconds= 0;
    m_DBHost        = "";
    m_DBPort        = 0;
    m_DBUsername    = "";
    m_DBPassword    = "";
    m_DBCharsets    = "";
    m_Database      = "";
}
