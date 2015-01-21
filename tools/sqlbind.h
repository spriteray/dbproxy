
#ifndef __SQLBIND_H__
#define __SQLBIND_H__

#include <vector>
#include <string>
#include <cstring>

#include <stdint.h>

namespace SQLBind
{

typedef std::vector<std::string>    Result;

class ISQLBind
{
public :
    ISQLBind() {}
    virtual ~ISQLBind() {}

    virtual std::string insert() = 0;
    virtual std::string query() = 0;
    virtual std::string update() = 0;
    virtual std::string remove() = 0;

    virtual bool parse( const Result & result ) = 0;
    virtual void autoincrease( const std::string & result ) = 0;
};

}

#endif
