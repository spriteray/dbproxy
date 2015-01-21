
#include <cstdio>
#include <cstring>

#include <sys/time.h>

#include "timeutils.h"

// 时间戳修正值
int32_t g_ModifyTimestamp = 0;

namespace Utils
{

TimeUtils::TimeUtils()
{
    m_Timestamp = 0;
    std::memset( &m_TimeStruct, 0, sizeof(m_TimeStruct) );
}

TimeUtils::TimeUtils( time_t t )
{
    m_Timestamp = t;
    ::localtime_r( &m_Timestamp, &m_TimeStruct );
}

TimeUtils::TimeUtils( struct tm * tm )
{
    m_TimeStruct = *tm;
    m_Timestamp = ::mktime( tm );
}

time_t TimeUtils::getTimestamp()
{
    if ( m_Timestamp == 0 )
    {
        m_Timestamp = TimeUtils::time();
    }

    return m_Timestamp;
}

struct tm * TimeUtils::getTimeStruct()
{
    if ( m_TimeStruct.tm_year == 0 )
    {
        time_t now = getTimestamp();
        ::localtime_r( &now, &m_TimeStruct );
    }

    return &m_TimeStruct;
}

int32_t TimeUtils::getDate()
{
    int32_t rc = 0;

    getTimeStruct();

    rc = ( m_TimeStruct.tm_year+1900 )*10000;
    rc += ( m_TimeStruct.tm_mon+1 )*100;
    rc += m_TimeStruct.tm_mday;

    return rc;
}

int32_t TimeUtils::getTime()
{
    int32_t rc = 0;

    getTimeStruct();

    rc = (m_TimeStruct.tm_hour)*10000;
    rc += (m_TimeStruct.tm_min)*100;
    rc += m_TimeStruct.tm_sec;

    return rc;
}

time_t TimeUtils::getZeroTimestamp()
{
    getTimeStruct();

    struct tm zero_timestamp = m_TimeStruct;
    zero_timestamp.tm_hour = zero_timestamp.tm_min = zero_timestamp.tm_sec = 0;

    return ::mktime( &zero_timestamp );
}

time_t TimeUtils::getNextZeroTimestamp()
{
    return (this->getZeroTimestamp()+24*60*60);
}

time_t TimeUtils::getSpecifiedTimestamp( const char * s )
{
    getTimeStruct();

    int32_t matched = 0;
    struct tm spec = m_TimeStruct;
    matched = std::sscanf( s, "%d:%d:%d", &(spec.tm_hour), &(spec.tm_min), &(spec.tm_sec) );

    return matched > 0 ? ::mktime( &spec ) : 0;
}

time_t TimeUtils::time()
{
    time_t now = 0;
    struct timeval tv;

    if ( ::gettimeofday(&tv, NULL) == 0 )
    {
        now = tv.tv_sec;
    }
    else
    {
        now = ::time( NULL );
    }

#if defined(__DEBUG__)
    now += g_ModifyTimestamp;
#endif

    return now;
}

int64_t TimeUtils::now()
{
    int64_t now = 0;
    struct timeval tv;

    if ( ::gettimeofday(&tv, NULL) == 0 )
    {
        now = tv.tv_sec*1000+tv.tv_usec/1000;
#if defined(__DEBUG__)
        now += (int64_t)g_ModifyTimestamp*1000;
#endif
    }

    return now;
}

int32_t TimeUtils::sleep( uint64_t mseconds )
{
    struct timeval tv;

    tv.tv_sec   = mseconds/1000;
    tv.tv_usec  = (mseconds%1000)*1000;

    return ::select(0, NULL, NULL, NULL, &tv);
}

time_t TimeUtils::getTimestamp( const char * str )
{
    struct tm t;
    char * matched = NULL;

    std::memset( &t, 0, sizeof(tm) );
    matched = strptime( str, "%Y-%m-%d %H:%M:%S", &t );

    return matched != NULL ? mktime(&t) : 0;
}

time_t TimeUtils::getTimestampByDate( const char * date )
{
    struct tm t;
    char * matched = NULL;

    std::memset( &t, 0, sizeof(tm) );
    matched = strptime( date, "%Y-%m-%d", &t );

    return matched != NULL ? mktime(&t) : 0;
}

}
