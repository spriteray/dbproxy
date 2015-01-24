
#ifndef __SRC_DBCLIENT_RESULTQUEUE_H__
#define __SRC_DBCLIENT_RESULTQUEUE_H__

#include <deque>

#include <pthread.h>

#include "define.h"
#include "message.h"

namespace Utils
{
    class TransactionManager;
};

class ResultQueue
{
public :
    ResultQueue();
    ~ResultQueue();

public :
    // 提交结果
    bool post( uint32_t transid, DBMessage * result );

    // 处理队列中的数据
    void process( Utils::TransactionManager * manager );

private :
    struct Task
    {
        uint32_t        transid;
        DBMessage *     message;

        Task( uint32_t id, DBMessage * result )
        {
            this->transid = id;
            this->message = result;
        }
    };

private :
    pthread_mutex_t     m_Lock;
    std::deque<Task>    m_Queue;
};

#endif
