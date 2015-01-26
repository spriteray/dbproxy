
#include <cstdio>
#include <cstdlib>

#include <stdint.h>

#include "utils/timeutils.h"
#include "dbclient/dbclient.h"

class CharacterCreateResult : public dbproxy::IDBResult
{
public :
    CharacterCreateResult()
        : m_Uid( 0ULL )
    {}

    virtual ~CharacterCreateResult()
    {}

    virtual void timeout()
    {
        printf( "CharacterCreateResult::timeout() .\n" );
    }

    virtual void autoincr( const std::string & result )
    {
        m_Uid = std::atoll( result.c_str() );
        printf( "CharacterCreateResult::autoincr( %lu ) .\n", m_Uid );
    }

    virtual void datasets( const std::vector<dbproxy::Result> & results )
    {}

private :
    uint64_t        m_Uid;
};

class CharacterLoginResult : public dbproxy::IDBResult
{
public :
    CharacterLoginResult()
        : m_Uid( 0ULL ),
          m_LoginStep( 0 )
    {}

    virtual ~CharacterLoginResult()
    {}

    virtual void timeout()
    {}

    virtual void autoincr( const std::string & result )
    {}

    virtual void datasets( const std::vector<dbproxy::Result> & results )
    {}

private :
    uint64_t        m_Uid;
    uint8_t         m_LoginStep;
};

#define PERCISION   20

int main()
{
    dbproxy::IDBClient * client = NULL;

    client = dbproxy::IDBClient::connect(
            "test", "127.0.0.1", 12000, 30, PERCISION, "log" );

    CharacterCreateResult * r1 = new CharacterCreateResult;
    client->insert( 0,
            "insert into game_role_info(account_id) values (34151)",
            r1, 10);

    // 更新
    //client->update(2, "update quest_info set q_commit_time = 0 where h_role_id=72057594037928063");

    // 批量查询
    CharacterLoginResult * r2 = new CharacterLoginResult;
    client->query(1, true,
            "select q_quest_id from quest_info where h_role_id=72057594037928062 and q_commit_time>0;\
             select q_quality, q_value_awards, q_item_awards from skybook_quest_info where h_role_id=72057594037928062;",
            r2, 10);

    while ( 1 )
    {
        client->schedule();
        Utils::TimeUtils::sleep( PERCISION );
    }

    delete client;
    return 0;
}
