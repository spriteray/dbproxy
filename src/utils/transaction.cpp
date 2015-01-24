
#include "transaction.h"

namespace Utils
{

Transaction::Transaction()
    : m_TransID( 0 ),
      m_Manager( NULL )
{}

Transaction::~Transaction()
{
    if ( m_TransID != 0 )
    {
        m_Manager->remove( m_TransID );
        m_TransID = 0;
    }
}

bool Transaction::onTimer( uint32_t & timeout )
{
    this->onTimeout();
    return false;
}

void Transaction::onEnd()
{
    // 移除事务
    m_Manager->remove( m_TransID );
    m_TransID = 0;
    delete this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TransactionManager::TransactionManager( TimeWheel * timer )
    : m_TimeDriver( timer ),
      m_UniqueTransID( 1 )
{}

TransactionManager::~TransactionManager()
{
    TransMap::iterator it;

    for ( it = m_TransMap.begin(); it != m_TransMap.end(); ++it )
    {
        Transaction * t = it->second;

        this->remove( it->first );
        delete t;
    }
}

TransID TransactionManager::append( Transaction * t, uint32_t seconds )
{
    while ( m_TransMap.find(m_UniqueTransID) != m_TransMap.end() )
    {
        ++m_UniqueTransID;

        if ( m_UniqueTransID == 0 )
        {
            m_UniqueTransID = 1;
        }
    }

    TransID id = m_UniqueTransID;
    t->setTransID( id );
    t->setManager( this );
    m_TimeDriver->schedule( t, seconds*1000 );
    m_TransMap.insert( std::make_pair(id, t) );

    ++m_UniqueTransID;

    return id;
}

Transaction * TransactionManager::get( TransID id )
{
    TransMap::iterator result;

    result = m_TransMap.find( id );
    if ( result != m_TransMap.end() )
    {
        return result->second;
    }

    return NULL;
}

bool TransactionManager::remove( Transaction * t )
{
    return remove( t->getTransID() );
}

bool TransactionManager::remove( TransID id )
{
    TransMap::iterator result;

    result = m_TransMap.find( id );
    if ( result == m_TransMap.end() )
    {
        return false;
    }

    Transaction * t = result->second;
    if ( t != NULL )
    {
        // 从定时器中取消
        m_TimeDriver->cancel( t );
    }

    // 从Map中移除
    m_TransMap.erase( result );
    return true;
}

}
