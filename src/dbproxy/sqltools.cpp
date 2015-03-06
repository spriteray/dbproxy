
#include <cstdlib>

#include "sqltools.h"

bool SqlTools::sqlbind( std::string & sqlcmd,
        const std::string & from, const std::vector<std::string> & values )
{
    size_t pos = 0;

    for ( size_t i = 0; i < from.size(); ++i )
    {
        if ( from[i] != '%' )
        {
            sqlcmd.push_back( from[i] );
            continue;
        }

        // 记录位置
        pos = ++i;
        // 查找匹配的%
        for ( ; from[i] != '%'; ++i );

        // 计算下标
        uint32_t id = std::atoi( std::string(from, pos, i-pos).c_str() );
        if ( id > values.size() )
        {
            sqlcmd = "";
            return false;
        }

        sqlcmd += values[ id-1 ];
    }

    return true;
}
