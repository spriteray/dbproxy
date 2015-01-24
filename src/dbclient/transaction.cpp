
#include "include/dbclient.h"
#include "message/proxy.pb.h"

#include "transaction.h"

DBResultTrans::DBResultTrans()
    : m_Result( NULL )
{}

DBResultTrans::~DBResultTrans()
{}

void DBResultTrans::onTimeout()
{
    m_Result->timeout();
}

void DBResultTrans::onTrigger( void * data )
{
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

        dbresults.push_back( dbresult );
    }

    m_Result->datasets( dbresults );
}

DBAutoIncrementTrans::DBAutoIncrementTrans()
    : m_Result( NULL )
{}

DBAutoIncrementTrans::~DBAutoIncrementTrans()
{}

void DBAutoIncrementTrans::onTimeout()
{
    m_Result->timeout();
}

void DBAutoIncrementTrans::onTrigger( void * data )
{
    msg::proxy::MessageAutoIncrement * result = (msg::proxy::MessageAutoIncrement *)data;

    m_Result->autoincr( result->insertid() );
}
