
#include <google/protobuf/message.h>

#include "utils/file.h"
#include "utils/streambuf.h"
#include "message/proxy.pb.h"

#include "clientimpl.h"
#include "agentclient.h"

CAgentClientSession::CAgentClientSession( CAgentClient * c )
    : m_AgentClient( c )
{}

CAgentClientSession::~CAgentClientSession()
{}

int32_t CAgentClientSession::onStart()
{
    uint32_t seconds = m_AgentClient->getTimeoutSeconds();

    // 设置超时时间
    this->setTimeout( seconds );
    // 设置KEEPALive时间
    this->setKeepalive( seconds/3 );

    return 0;
}

int32_t CAgentClientSession::onProcess( const char * buffer, uint32_t nbytes )
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

int32_t CAgentClientSession::onTimeout()
{
    m_AgentClient->setStatus( eReconnecting );
    m_AgentClient->getHandler()->getLogger()->print(
            0, "CAgentClientSession::onTimeout(Sid:%lu) .\n", id() );
    return 0;
}

int32_t CAgentClientSession::onKeepalive()
{
    msg::proxy::MessagePingCmd msg;
    m_AgentClient->send( msg::proxy::eMessage_Ping, 0, &msg );
    return 0;
}

int32_t CAgentClientSession::onError( int32_t result )
{
    m_AgentClient->setStatus( eReconnecting );
    m_AgentClient->getHandler()->getLogger()->print(
            0, "CAgentClientSession::onError(Sid:%lu) .\n", id() );
    return 0;
}

void CAgentClientSession::onShutdown( int32_t way )
{
    m_AgentClient->setStatus( eDisconnected );
    m_AgentClient->getHandler()->getLogger()->print(
            0, "CAgentClientSession::onShutdown(Sid:%lu, Way:%d) .\n", id(), way );
}

void CAgentClientSession::onMessage( DBHead * head, const char * buf )
{
    DBMessage * msg = NULL;

    if ( head->cmd == msg::proxy::eMessage_Ping )
    {
        return;
    }

    switch ( head->cmd )
    {
        case msg::proxy::eMessage_AutoIncrease :
            {
                msg = new msg::proxy::MessageAutoIncrement;
            }
            break;

        case msg::proxy::eMessage_Results :
            {
                msg = new msg::proxy::MessageResultSets;
            }
            break;

        default :
            m_AgentClient->getHandler()->getLogger()->print( 0,
                    "CAgentClientSession::onMessage(Sid:%lu, TransID:%u) : the CMD(0x%04x) can't Register .\n",
                    id(), head->transid, head->cmd );
            break;
    }

    if ( msg )
    {
        msg->ParseFromArray( buf, head->len );
        m_AgentClient->getHandler()->getResultQueue()->post( head->transid, msg );
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CAgentClient::CAgentClient( ClientImpl * handler, uint32_t timeout )
    : IIOService( 1, 32 ),
      m_Sid( 0ULL ),
      m_Status( eConnecting ),
      m_Timeout( timeout ),
      m_Handler( handler )
{}

CAgentClient::~CAgentClient()
{}

IIOSession * CAgentClient::onConnect( sid_t id, const char * host, uint16_t port )
{
    m_Sid = id;
    m_Status = eConnected;

    return new CAgentClientSession( this );
}

int32_t CAgentClient::send( uint16_t cmd, uint32_t transid, DBMessage * msg )
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

    return IIOService::send( m_Sid, buffer );
}
