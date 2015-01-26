
#include "utils/file.h"

#include "include/dbclient.h"
#include "message/proxy.pb.h"

#include "clientimpl.h"
#include "transaction.h"

DBResultTrans::DBResultTrans()
    : m_Client( NULL ),
      m_Result( NULL )
{}

DBResultTrans::~DBResultTrans()
{}

void DBResultTrans::onTimeout()
{
    m_Client->getLogger()->print( 0, "DBResultTrans::onTimeout(TransID:%u) .\n", this->getTransID() );
    m_Result->timeout();
}

void DBResultTrans::onTrigger( void * data )
{
    size_t nfields = 0;
    std::vector<dbproxy::Result> dbresults;

    msg::proxy::MessageResultSets * result = (msg::proxy::MessageResultSets *)data;

    for ( int32_t i = 0; i < result->results_size(); ++i )
    {
        dbproxy::Result dbresult;
        msg::proxy::MessageResultSets::Result * r = result->mutable_results( i );

        for ( int32_t j = 0; j < r->field_size(); ++j )
        {
            dbresult.push_back( r->field(j) );
        }

        if ( i == 0 )
        {
            nfields = dbresult.size();
        }

        dbresults.push_back( dbresult );
    }

    m_Client->getLogger()->print(
            0, "DBResultTrans::onTrigger(TransID:%u) : this ResultSize(Rows:%lu) is %lu .\n",
            this->getTransID(), nfields, dbresults.size() );
    m_Result->datasets( dbresults );
}

DBAutoIncrementTrans::DBAutoIncrementTrans()
    : m_Client( NULL ),
      m_Result( NULL )
{}

DBAutoIncrementTrans::~DBAutoIncrementTrans()
{}

void DBAutoIncrementTrans::onTimeout()
{
    m_Client->getLogger()->print( 0, "DBAutoIncrementTrans::onTimeout(TransID:%u) .\n", this->getTransID() );
    m_Result->timeout();
}

void DBAutoIncrementTrans::onTrigger( void * data )
{
    msg::proxy::MessageAutoIncrement * result = (msg::proxy::MessageAutoIncrement *)data;

    m_Client->getLogger()->print(
            0, "DBAutoIncrementTrans::onTrigger(TransID:%u) : this InsertID is %s .\n",
            this->getTransID(), result->insertid().c_str() );
    m_Result->autoincr( result->insertid() );
}
