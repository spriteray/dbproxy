
#include "utils/transaction.h"

#include "resultqueue.h"

ResultQueue::ResultQueue()
{
    pthread_mutex_init( &m_Lock, NULL );
}

ResultQueue::~ResultQueue()
{}

bool ResultQueue::post( uint32_t transid, DBMessage * result )
{
    pthread_mutex_lock( &m_Lock );
    m_Queue.push_back( Task(transid, result) );
    pthread_mutex_unlock( &m_Lock );

    return true;
}

void ResultQueue::process( Utils::TransactionManager * manager )
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
        Utils::Transaction * trans = manager->get( iter->transid );
        if ( trans )
        {
            trans->onTrigger( iter->message );
            delete trans;
        }
    }
}
