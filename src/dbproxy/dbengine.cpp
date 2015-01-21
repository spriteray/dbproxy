
#include "base.h"
#include "dbengine.h"

DBEngine::DBEngine( const std::string & host, uint16_t port )
    : m_Host( host ),
      m_Port( port ),
      m_DBHandler( NULL )
{}

DBEngine::~DBEngine()
{}

void DBEngine::selectdb( const std::string & database )
{
    m_Database = database;
}

void DBEngine::setCharsets( const std::string & charsets )
{
    m_Charsets = charsets;
}

void DBEngine::setToken( const std::string & user, const std::string & passwd )
{
    m_Username = user;
    m_Password = passwd;
}

bool DBEngine::initialize()
{
    // 初始化句柄
    m_DBHandler = mysql_init( NULL );
    if ( m_DBHandler == NULL )
    {
        return false;
    }

    // 设置属性
    bool flags = true;
    int32_t timeout = 0;

    // 尝试重新连接
    mysql_options( m_DBHandler, MYSQL_OPT_RECONNECT, (const char *)&flags );
    // 设置字符集
    mysql_options( m_DBHandler, MYSQL_SET_CHARSET_NAME, m_Charsets.c_str() );
    // 连接超时
    timeout = eDBEngine_ConnectTimeout;
    mysql_options( m_DBHandler, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout );
    // 读超时
    timeout = eDBEngine_ReadTimeout;
    mysql_options( m_DBHandler, MYSQL_OPT_READ_TIMEOUT, (const char *)&timeout );
    // 写超时
    timeout = eDBEngine_WriteTimeout;
    mysql_options( m_DBHandler, MYSQL_OPT_WRITE_TIMEOUT, (const char *)&timeout );

    // 连接MySQL服务器
    if ( mysql_real_connect( m_DBHandler,
                m_Host.c_str(), m_Username.c_str(), m_Password.c_str(),
                m_Database.c_str(), m_Port, NULL, 0 ) == NULL )
    {
        mysql_close( m_DBHandler );
        m_DBHandler = NULL;
        return false;
    }

    return true;
}

void DBEngine::finalize()
{
    if ( m_DBHandler != NULL )
    {
        mysql_close( m_DBHandler );
        m_DBHandler = NULL;
    }
}

void DBEngine::keepalive()
{
    int32_t rc = 0;

    rc = mysql_ping( m_DBHandler );
    if ( rc != 0 )
    {
        LOG_ERROR( "DBEngine::keepalive() : Result:%d, Error:(%d,'%s') .\n",
                rc, mysql_errno(m_DBHandler), mysql_error(m_DBHandler) );
    }
}

bool DBEngine::insert( const std::string & sqlcmd, uint64_t & insertid )
{
    int32_t rc = 0;

    rc = mysql_real_query( m_DBHandler, sqlcmd.c_str(), sqlcmd.size() );
    if ( rc != 0 )
    {
        LOG_ERROR( "DBEngine::insert(SQLCMD:'%s') : Result:%d, Error:(%d,'%s') .\n",
                sqlcmd.c_str(), rc, mysql_errno(m_DBHandler), mysql_error(m_DBHandler) );
        return false;
    }

    // 获取返回的自增ID
    insertid = mysql_insert_id( m_DBHandler );
    LOG_TRACE( "DBEngine::insert(SQLCMD:'%s') : InsertID:%lu .\n", sqlcmd.c_str(), insertid );

    return true;
}

bool DBEngine::query( const std::string & sqlcmd, Results & results )
{
    int32_t rc = 0;

    rc = mysql_real_query( m_DBHandler, sqlcmd.c_str(), sqlcmd.size() );
    if ( rc != 0 )
    {
        LOG_ERROR( "DBEngine::query(SQLCMD:'%s') : Result:%d, Error:(%d,'%s') .\n",
                sqlcmd.c_str(), rc, mysql_errno(m_DBHandler), mysql_error(m_DBHandler) );
        return false;
    }

    MYSQL_RES * result = mysql_store_result( m_DBHandler );
    if ( result == NULL )
    {
        LOG_ERROR( "DBEngine::query(SQLCMD:'%s') : Store Result Failed .\n", sqlcmd.c_str() );
        return false;
    }

    MYSQL_ROW row;
    uint32_t nfields = mysql_num_fields( result );
    while ( (row = mysql_fetch_row(result)) != NULL )
    {
        Result tmpresult;
        uint64_t * lengths = mysql_fetch_lengths( result );

        for ( uint32_t i = 0; i < nfields; ++i )
        {
            tmpresult.push_back( std::string( row[i], lengths[i] ) );
        }

        results.push_back( tmpresult );
    }

    mysql_free_result( result );
    LOG_TRACE( "DBEngine::query(SQLCMD:'%s') : ResultCount:%lu .\n", sqlcmd.c_str(), results.size() );

    return true;
}

bool DBEngine::update( const std::string & sqlcmd, uint32_t & naffected )
{
    int32_t rc = 0;

    rc = mysql_real_query( m_DBHandler, sqlcmd.c_str(), sqlcmd.size() );
    if ( rc != 0 )
    {
        LOG_ERROR( "DBEngine::update(SQLCMD:'%s') : Result:%d, Error:(%d,'%s') .\n",
                sqlcmd.c_str(), rc, mysql_errno(m_DBHandler), mysql_error(m_DBHandler) );
        return false;
    }

    // 获取影响的行数
    naffected = mysql_affected_rows( m_DBHandler );
    LOG_TRACE( "DBEngine::update(SQLCMD:'%s') : AffectedRows:%u .\n", sqlcmd.c_str(), naffected );

    return true;
}

bool DBEngine::remove( const std::string & sqlcmd, uint32_t & naffected )
{
    int32_t rc = 0;

    rc = mysql_real_query( m_DBHandler, sqlcmd.c_str(), sqlcmd.size() );
    if ( rc != 0 )
    {
        LOG_ERROR( "DBEngine::remove(SQLCMD:'%s') : Result:%d, Error:(%d,'%s') .\n",
                sqlcmd.c_str(), rc, mysql_errno(m_DBHandler), mysql_error(m_DBHandler) );
        return false;
    }

    // 获取影响的行数
    naffected = mysql_affected_rows( m_DBHandler );
    LOG_TRACE( "DBEngine::remove(SQLCMD:'%s') : AffectedRows:%u .\n", sqlcmd.c_str(), naffected );

    return true;
}
