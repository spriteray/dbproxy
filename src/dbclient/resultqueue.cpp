
#include "utils/file.h"
#include "utils/transaction.h"

#include "clientimpl.h"
#include "resultqueue.h"

ResultQueue::ResultQueue()
{
    pthread_mutex_init( &m_Lock, NULL );
}

ResultQueue::~ResultQueue()
{
    pthread_mutex_destroy( &m_Lock );
}

bool ResultQueue::post( uint32_t transid, DBMessage * result )
{
    pthread_mutex_lock( &m_Lock );
    m_Queue.push_back( Task(transid, result) );
    pthread_mutex_unlock( &m_Lock );

    return true;
}

void ResultQueue::process( ClientImpl * client )
{
    // swap
    std::deque<Task> taskqueue;
    pthread_mutex_lock( &m_Lock );
    std::swap( taskqueue, m_Queue );
    pthread_mutex_unlock( &m_Lock );

    // loop, and process
    std::deque<Task>::iterator iter;
    for ( iter = taskqueue.begin(); iter != taskqueue.end(); ++iter )
    {
        Utils::LogFile * logger = client->getLogger();
        Utils::TransactionManager * manager = client->getTransManager();

        Utils::Transaction * trans = manager->get( iter->transid );
        if ( trans == NULL )
        {
            logger->print(
                    0, "ResultQueue::process(TransID:%u) : this Transaction can't found .\n", iter->transid );
            continue;
        }

        trans->onTrigger( iter->message );
        delete trans;
    }
}
