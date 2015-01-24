
#ifndef __SRC_UTILS_TRANSACTION_H__
#define __SRC_UTILS_TRANSACTION_H__

#include <map>
#include <stdint.h>

#include "timer.h"

typedef uint32_t    TransID;

namespace Utils
{

class TransactionManager;

class Transaction : public ITimerEvent
{
public :
    Transaction();
    virtual ~Transaction();

    // 事务超时事件
    virtual void onTimeout() = 0;
    // 事务触发事件
    virtual void onTrigger( void * data ) = 0;

public :
    // 设置/获取TransID
    TransID getTransID() const { return m_TransID; }
    void setTransID( TransID id ) { m_TransID = id; }

    // 设置管理器
    void setManager( TransactionManager * m ) { m_Manager = m; }

private :
    // 定时器接口
    bool onTimer( uint32_t & timeout );
    void onEnd();

private :
    TransID                 m_TransID;
    TransactionManager *    m_Manager;
};

class TransactionManager
{
public :
    TransactionManager( TimeWheel * timer );
    ~TransactionManager();

public :
    // 添加事务
    TransID append( Transaction * t, uint32_t seconds );

    // 获取事务
    Transaction * get( TransID id );

    // 移除事务
    bool remove( Transaction * t );
    bool remove( TransID id );

private :
    typedef std::map<TransID, Transaction *> TransMap;

    TransMap    m_TransMap;
    TimeWheel * m_TimeDriver;
    TransID     m_UniqueTransID;
};

}

#endif
