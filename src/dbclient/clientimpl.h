
#ifndef __SRC_DBCLIENT_CLIENTIMPL_H__
#define __SRC_DBCLIENT_CLIENTIMPL_H__

#include <string>

#include "resultqueue.h"
#include "include/dbclient.h"

class TimeTick;
class CAgentClient;

namespace Utils
{
    class LogFile;
    class TransactionManager;
}

class ClientImpl : public dbproxy::IDBClient
{
public :
    ClientImpl( const char * tag );
    virtual ~ClientImpl();

public :
    // 调度/处理结果
    virtual void schedule();

    // 执行更新语句
    virtual void update(
            uint8_t index,
            const std::string & sqlcmd );

    // 执行删除语句
    virtual void remove(
            uint8_t index,
            const std::string & sqlcmd );

    // 执行插入语句
    virtual void insert(
            uint8_t index,
            const std::string & sqlcmd,
            dbproxy::IDBResult * result, uint32_t timeout );

    // 执行查询语句
    virtual void query(
            uint8_t index,
            bool isbatch,
            const std::string & sqlcmd,
            dbproxy::IDBResult * result, uint32_t timeout );

public :
    // 获取连接状态
    uint8_t status() const;

    // 初始化和销毁
    bool initialize( const char * path );
    void finalize();

    // 设置超时时间
    void setTimeout( uint32_t timeout );
    // 设置定时器的精度
    void setPercision( uint32_t percision );
    // 设置Proxy的网络信息
    void setEndpoint( const char * host, uint16_t port );

    // 结果队列
    ResultQueue * getResultQueue() { return &m_Results; }
    // 获取日志句柄
    Utils::LogFile * getLogger() const { return m_Logger; }
    // 获取事务管理器
    Utils::TransactionManager * getTransManager() const { return m_TransactionManager; }

private :
    std::string         m_Tag;
    uint32_t            m_Timeout;
    uint32_t            m_Persicion;
    std::string         m_ProxyHost;
    uint16_t            m_ProxyPort;
    uint64_t            m_ScheduleTime;

private :
    ResultQueue                     m_Results;              // 查询结果
    Utils::LogFile *                m_Logger;               // 日志系统
    CAgentClient *                  m_Client;               // 网络线程
    TimeTick *                      m_TimeTick;             // 定时器
    Utils::TransactionManager *     m_TransactionManager;   // 事务管理器
};

#endif
