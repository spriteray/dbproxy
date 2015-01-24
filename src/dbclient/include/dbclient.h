
#ifndef __DBPROXY_CLIENT_H__
#define __DBPROXY_CLIENT_H__

#include <vector>
#include <string>
#include <stdint.h>

namespace dbproxy
{

typedef std::vector<std::string> Result;

class IDBResult
{
public :
    IDBResult() {}
    virtual ~IDBResult() {}

    // 超时
    virtual void timeout() = 0;

    // 自增ID返回
    virtual void autoincr( const std::string & result ) = 0;

    // 查询结果返回
    virtual void datasets( const std::vector<Result> & results ) = 0;
};

class IDBClient
{
public :
    IDBClient() {}
    virtual ~IDBClient() {}

    // 连接远程服务器
    // host         - DBProxy服务器主机
    // port         - DBProxy服务器端口
    // timeout      - 超时时间
    // threads      - DB线程个数
    // percision    - schedule()线程调度进度
    static IDBClient * connect(
            const char * host, uint16_t port,
            uint32_t timeout, uint32_t percition );

public :
    // 调度/处理结果, 以相同的频率调用
    // 建议每帧调用一次
    virtual void schedule() = 0;

    // 执行更新语句
    virtual void update(
            uint8_t index,
            const std::string & sqlcmd ) = 0;

    // 执行删除语句
    virtual void remove(
            uint8_t index,
            const std::string & sqlcmd ) = 0;

    // 执行插入语句
    virtual void insert(
            uint8_t index,
            const std::string & sqlcmd,
            IDBResult * result, uint32_t timeout ) = 0;

    // 执行查询语句
    virtual void query(
            uint8_t index,
            bool isbatch,
            const std::string & sqlcmd,
            IDBResult * result, uint32_t timeout ) = 0;
};

}

#endif
