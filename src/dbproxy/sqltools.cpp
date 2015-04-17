
#include <cstdlib>

#include "sqltools.h"

// INSERT xxx ( `id`, `sender`, `content` ) VALUES ( 1234, 'lei.zhang', '?' ) ON DUPLICATE KEY UPDATE `sender`='lei.zhang', `content`='?'"

bool SqlTools::sqlbind( std::string & sqlcmd,
        const std::string & from, const std::vector<std::string> & values )
{
    size_t index = 0;

    for ( size_t i = 0; i < from.size(); ++i )
    {
        if ( from[i] != '?' )
        {
            sqlcmd.push_back( from[i] );
            continue;
        }

        if ( index >= values.size() )
        {
            sqlcmd.clear();
            return false;
        }

        sqlcmd += values[ index++ ];
    }

    return true;
}
