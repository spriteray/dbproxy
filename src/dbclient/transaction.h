
#ifndef __SRC_DBCLIENT_TRANSACTION_H__
#define __SRC_DBCLIENT_TRANSACTION_H__

#include "utils/transaction.h"

namespace dbproxy
{
    class IDBResult;
}

class ClientImpl;

class DBResultTrans: public Utils::Transaction
{
public :
    DBResultTrans();
    virtual ~DBResultTrans();

    virtual void onTimeout();
    virtual void onTrigger( void * data );

public :
    void setClient( ClientImpl * cli ) { m_Client = cli; }
    void setResult( dbproxy::IDBResult * result ) { m_Result = result; }

private :
    ClientImpl *                m_Client;
    dbproxy::IDBResult *        m_Result;
};

class DBAutoIncrementTrans : public Utils::Transaction
{
public :
    DBAutoIncrementTrans();
    virtual ~DBAutoIncrementTrans();

    virtual void onTimeout();
    virtual void onTrigger( void * data );

public :
    void setClient( ClientImpl * cli ) { m_Client = cli; }
    void setResult( dbproxy::IDBResult * result ) { m_Result = result; }

private :
    ClientImpl *                m_Client;
    dbproxy::IDBResult *        m_Result;
};

#endif
