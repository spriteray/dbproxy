
#include <google/protobuf/message.h>

#include "base.h"
#include "utils/streambuf.h"

#include "message/proxy.pb.h"

#include "config.h"
#include "dbthreads.h"
#include "agentservice.h"

CDBClientSession::CDBClientSession( const char * host, uint16_t port )
    : m_Host( host ),
      m_Port( port )
{}

CDBClientSession::~CDBClientSession()
{}

int32_t CDBClientSession::onStart()
{
    this->setTimeout( CDBProxyConfig::getInstance().getTimeoutSeconds() );
    return 0;
}

int32_t CDBClientSession::onProcess( const char * buffer, uint32_t nbytes )
{
    int32_t nprocess = 0;

    for ( ;; )
    {
        uint32_t nleft = nbytes - nprocess;
        const char * buf = buffer + nprocess;

        if ( nleft < sizeof(DBHead) )
        {
            break;
        }

        DBHead head;
        StreamBuf unpack( StreamBuf::eMethod_Decode, (char *)buf, nleft );
        unpack.code( head.cmd );
        unpack.code( head.len );
        unpack.code( head.transid );

        if ( nleft < head.len +sizeof(DBHead) )
        {
            break;
        }

        // 处理消息
        this->onMessage( &head, buf+sizeof(DBHead) );

        nprocess += head.len;
        nprocess += sizeof(DBHead);
    }

    return nprocess;
}

int32_t CDBClientSession::onTimeout()
{
    LOG_WARN( "CDBClientSession::onTimeout(%lu, %s::%s) .\n",
            id(), m_Host.c_str(), m_Port );
    return -1;
}

int32_t CDBClientSession::onError( int32_t result )
{
    LOG_WARN( "CDBClientSession::onError(%lu, %s::%s) : 0x%08x .\n",
            id(), m_Host.c_str(), m_Port, result );
    return -1;
}

void CDBClientSession::onShutdown( int32_t way )
{
    LOG_WARN( "CDBClientSession::onShutdown(%lu, %s::%s) : way=%d .\n",
            id(), m_Host.c_str(), m_Port, way );
}

void CDBClientSession::onMessage( DBHead * head, const char * buf )
{
    uint32_t index = 0;
    std::string sqlcmd;
    SQLCmd cmd = eSQLCmd_None;

    switch ( head->cmd )
    {
        case msg::proxy::eMessage_Insert :
            {
                msg::proxy::MessageInsertSqlCmd msg;

                if ( msg.ParseFromArray( buf, head->len ) )
                {
                    index = msg.index();
                    cmd = eSQLCmd_Insert;
                    sqlcmd = msg.sqlcmd();
                }
            }
            break;

        case msg::proxy::eMessage_Query :
            {
                msg::proxy::MessageQuerySqlCmd msg;

                if ( msg.ParseFromArray( buf, head->len ) )
                {
                    index = msg.index();
                    cmd = eSQLCmd_Query;
                    sqlcmd = msg.sqlcmd();
                }
            }
            break;

        case msg::proxy::eMessage_Update :
            {
                msg::proxy::MessageUpdateSqlCmd msg;

                if ( msg.ParseFromArray( buf, head->len ) )
                {
                    index = msg.index();
                    cmd = eSQLCmd_Update;
                    sqlcmd = msg.sqlcmd();
                }
            }
            break;

        case msg::proxy::eMessage_Remove :
            {
                msg::proxy::MessageRemoveSqlCmd msg;

                if ( msg.ParseFromArray( buf, head->len ) )
                {
                    index = msg.index();
                    cmd = eSQLCmd_Remove;
                    sqlcmd = msg.sqlcmd();
                }
            }
            break;
    }

    if ( sqlcmd.empty() || cmd == eSQLCmd_None )
    {
        // TODO: 日志
        return;
    }

    g_DBThreads->post( index, cmd, id(), head->transid, sqlcmd );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CAgentService::CAgentService( uint8_t nthreads, uint32_t nclients )
    : IIOService( nthreads, nclients ),
      m_ThreadsCount( nthreads )
{
    // TODO:
}

CAgentService::~CAgentService()
{
    // TODO:
}

void * CAgentService::getLocalData( uint8_t index )
{
    // TODO:
    return NULL;
}

IIOSession * CAgentService::onAccept( sid_t sid, const char * host, uint16_t port )
{
    return new CDBClientSession( host, port );
}

int32_t CAgentService::send( sid_t id, uint32_t transid, uint16_t cmd, DBMessage * msg )
{
    uint32_t length = msg->ByteSize();

    std::string buffer( length+sizeof(DBHead)+1, 0 );
    char * head = (char *)buffer.data();
    uint8_t * body = (uint8_t *)( head + sizeof(DBHead) );

    StreamBuf pack( StreamBuf::eMethod_Encode, head, sizeof(DBHead) );
    pack.code( cmd );
    pack.code( length );
    pack.code( transid );

    msg->SerializeWithCachedSizesToArray( body );
    buffer.resize( length+sizeof(DBHead) );

    return IIOService::send( id, buffer );
}
