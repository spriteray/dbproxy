
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <stdarg.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "utility.h"

namespace Utils
{

RandomDevice Utility::g_Device;
Random Utility::g_Generate( Utility::g_Device.get() );

bool Utility::mkdirp( const char * path )
{
    int32_t len = std::strlen(path);
    char * p = (char *)std::malloc( len+2 );

    if ( p == NULL )
    {
        return false;
    }

    std::strcpy( p, path );

    if ( p[ len-1 ] != '/' )
    {
        p[ len ] = '/';
    }

    for ( int32_t i = 1; i < len; ++i )
    {
        if ( p[i] != '/' )
        {
            continue;
        }

        p[ i ] = 0;

        if ( ::access( p, F_OK ) != 0 )
        {
            ::mkdir( p, 0755 );
        }

        p[ i ] = '/';
    }

    std::free( p );
    return true;
}

void Utility::trim( std::string & str )
{
    str.erase( 0, str.find_first_not_of(" ") );
    str.erase( str.find_last_not_of(" ") + 1 );
}

int32_t Utility::snprintf( std::string & str, size_t size, const char * format, ... )
{
    str.resize( size );

    va_list args;
    va_start( args, format );
    int32_t rc = vsnprintf(
            const_cast<char *>( str.data() ), size-1, format, args );
    if ( rc >= 0 && rc <= (int32_t)( size - 1 ) )
    {
        str.resize( rc );
    }
    va_end( args );

    return rc;
}

}
