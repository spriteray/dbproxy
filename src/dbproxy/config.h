
#ifndef __SRC_DBPROXY_CONFIG_H__
#define __SRC_DBPROXY_CONFIG_H__

#include "define.h"
#include "utils/singleton.h"

class CDBProxyConfig : public Singleton<CDBProxyConfig>
{
public :
    CDBProxyConfig();
    ~CDBProxyConfig();

public :
    bool load( const char * path );
    bool reload( const char * path );
    void unload();

public :
    uint8_t getLogLevel() const { return m_LogLevel; }
    uint8_t getDBThreads() const { return m_DBThreads; }
    size_t getLogFilesize() const { return m_LogFilesize; }

public :
    // Service
    const std::string & getBindHost() const { return m_BindHost; }
    uint16_t getListenPort() const { return m_ListenPort; }
    uint32_t getMaxClients() const { return m_MaxClients; }
    uint8_t getServiceThreads() const { return m_ServiceThreads; }
    uint32_t getTimeoutSeconds() const { return m_Timeoutseconds; }

public :
    // Database
    const std::string & getDBHost() const { return m_DBHost; }
    uint16_t getDBPort() const { return m_DBPort; }
    const std::string & getDatabase() const { return m_Database; }
    const std::string & getDBUsername() const { return m_DBUsername; }
    const std::string & getDBPassword() const { return m_DBPassword; }
    const std::string & getDBCharsets() const { return m_DBCharsets; }

private :
    uint8_t         m_LogLevel;         // 日志等级
    uint8_t         m_DBThreads;        // DB线程个数
    size_t          m_LogFilesize;      // 日志文件大小限制

private :
    std::string     m_BindHost;         // 绑定的IP地址
    uint16_t        m_ListenPort;       // 监听的端口号
    uint32_t        m_MaxClients;       // 最大客户端数目
    uint8_t         m_ServiceThreads;   // 服务线程个数
    uint32_t        m_Timeoutseconds;   // 连接的超时时间

private :
    std::string     m_DBHost;           // 数据库地址
    uint16_t        m_DBPort;           // 数据库端口
    std::string     m_DBUsername;       // 数据库用户名
    std::string     m_DBPassword;       // 数据库密码
    std::string     m_DBCharsets;       // 连接编码
    std::string     m_Database;         // 访问的库名
};

#endif
