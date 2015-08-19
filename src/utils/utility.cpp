
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
        len = len + 1;
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

char * Utility::strsep( char ** s, const char *del )
{
    char * d, * tok;

    if ( !s || !*s )
    {
        return NULL;
    }

    tok = *s;
    d = std::strstr( tok, del );

    if ( d )
    {
        *s = d + std::strlen( del );
        *d = '\0';
    }
    else
    {
        *s = NULL;
    }

    return tok;
}

void Utility::randstring( size_t len, std::string & value )
{
    int32_t type = 0;

    // 清空字符串
    value.clear();

    for ( size_t i = 0; i < len; ++i )
    {
        type = g_Generate.rand() % 3;

        switch ( type )
        {
            case 0 :
                value.push_back( (char)( g_Generate.rand()%26 + 'a' ) );
                break;

            case 1 :
                value.push_back( (char)( g_Generate.rand()%26 + 'A' ) );
                break;

            case 2 :
                value.push_back( (char)( g_Generate.rand()%10 + '0' ) );
                break;
        }
    }
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

bool Utility::replace( std::string & dst,
        const std::string & src, const std::string & sub, const std::vector<std::string> & values )
{
    size_t index = 0;

    for ( size_t i = 0; i < src.size(); )
    {
        if ( src.substr( i, sub.size() ) != sub )
        {
            // 没有匹配
            // TODO: 此处可以参考KMP算法进行优化
            dst.push_back( src[i++] );
            continue;
        }

        if ( index >= values.size() )
        {
            dst.clear();
            return false;
        }

        i += sub.size();
        dst += values[ index++ ];
    }

    return true;
}

}
