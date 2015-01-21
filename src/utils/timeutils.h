
#ifndef __SRC_UTILS_TIME_H__
#define __SRC_UTILS_TIME_H__

#include <time.h>
#include <stdint.h>

// 全局事件修正
extern int32_t g_ModifyTimestamp;

namespace Utils
{

class TimeUtils
{
public:
    TimeUtils();
    TimeUtils( time_t t );
    TimeUtils( struct tm * tm );

public :
    // 获取UTC时间
    time_t getTimestamp();

    // 获取时间结构
    struct tm * getTimeStruct();

public :
    enum
    {
        eSeconds_OneDay        = 3600*24,    // 一天的秒数
    };

    // 2012-12-24 20121224
    int32_t getDate();

    // 10:10:58 101058
    int32_t getTime();

    // 获取凌晨零点的时间
    time_t getZeroTimestamp();

    // 获取下一天零点的时间
    time_t getNextZeroTimestamp();

    // 获取当天指定时间点的时间戳
    // 10:00:00
    time_t getSpecifiedTimestamp( const char * s );

public :
    // 获得当前时间的秒数
    static time_t time();

    // 获得当前时间的毫秒数
    static int64_t now();

    // 毫秒
    static int32_t sleep( uint64_t mseconds );

    // "2010-10-10 00:10:01"
    static time_t getTimestamp( const char * str );

    // "2010-10-10"
    static time_t getTimestampByDate( const char * date );

private :
    time_t        m_Timestamp;
    struct tm     m_TimeStruct;
};

}

#endif
