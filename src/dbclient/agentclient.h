
#ifndef __SRC_DBCLIENT_AGENTCLIENT_H__
#define __SRC_DBCLIENT_AGENTCLIENT_H__

#include "io/io.h"
#include "message.h"

enum
{
    eInvalid        = 0,    // 非法
    eConnecting     = 1,    // 连接中
    eConnected      = 2,    // 正在连接
    eReconnecting   = 3,    // 重连中
    eDisconnected   = 4,    // 断开
};

class ResultQueue;
class CAgentClient;

class CAgentClientSession : public IIOSession
{
public :
    CAgentClientSession( CAgentClient * c );
    virtual ~CAgentClientSession();

    int32_t onStart();
    int32_t onProcess( const char * buffer, uint32_t nbytes );
    int32_t onTimeout();
    int32_t onKeepalive();
    int32_t onError( int32_t result );
    void    onShutdown( int32_t way );

private :
    void onMessage( DBHead * head, const char * buf );

private :
    CAgentClient *  m_AgentClient;
};

class CAgentClient : public IIOService
{
public :
    CAgentClient( ResultQueue * queue, uint32_t timeout );
    virtual ~CAgentClient();

    virtual IIOSession * onConnect( sid_t id, const char * host, uint16_t port );

public :
    // 获取会话ID
    sid_t getSid() const { return m_Sid; }

    // 获取客户端状态
    uint8_t getStatus() const { return m_Status; }
    void setStatus( uint8_t status ) { m_Status = status; }

    // 获取超时时间
    uint32_t getTimeoutSeconds() const { return m_Timeout; }

    // 获取结果队列
    ResultQueue * getResultQueue() const { return m_ResultQueue; }

    // 发送数据
    int32_t send( uint16_t cmd, uint32_t transid, DBMessage * msg );

private :
    sid_t           m_Sid;
    uint8_t         m_Status;
    uint32_t        m_Timeout;

    ResultQueue *   m_ResultQueue;
};

#endif
