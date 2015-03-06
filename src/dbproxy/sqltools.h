
#ifndef __SRC_DBPROXY_SQLTOOLS_H__
#define __SRC_DBPROXY_SQLTOOLS_H__

#include <stdint.h>

#include <vector>
#include <string>

class SqlTools
{
public :
    // %ID%
    static bool sqlbind( std::string & sqlcmd,
            const std::string & from, const std::vector<std::string> & values );
};

#endif
