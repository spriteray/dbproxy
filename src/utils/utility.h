
#ifndef __SRC_UTILS_UTILITY_H__
#define __SRC_UTILS_UTILITY_H__

#include <vector>
#include <string>

#include <stddef.h>
#include <stdint.h>

#include "random.h"

namespace Utils
{

class Utility
{
public :
    // 建立多级目录
    // 模拟mkdir -p的功能
    static bool mkdirp( const char * p );

    // 去除空格
    static void trim( std::string & str );

    // strsep
    static char * strsep( char ** s, const char * del );

    // 随机排序
    template<class T>
        static void shuffle( std::vector<T> & array )
        {
            size_t count = array.size();

            for ( size_t i = 0; i < count; ++i )
            {
                size_t index = g_Generate.rand()%count;
                std::swap( array[i], array[index] );
            }

            return;
        }

    // 随机字符串
    static void randstring( size_t len, std::string & value );

    // 字符串sprintf
    static int32_t snprintf( std::string & dst, size_t size, const char * format, ... );

    // 替换字符串
    static bool replace( std::string & dst,
            const std::string & src, const std::string & sub, const std::vector<std::string> & values );

private :
    static RandomDevice g_Device;
    static Random       g_Generate;
};

}

#endif
