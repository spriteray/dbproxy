
#ifndef __SRC_UTILS_FILE_H__
#define __SRC_UTILS_FILE_H__

#include <map>
#include <string>

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

namespace Utils
{

//
// 配置文件, INI文件格式
//
class ConfigSection;
class ConfigFile
{
public :
    ConfigFile( const char * path );
    ~ConfigFile();

public :
    // 打开配置文件
    bool open();

    // 关闭配置文件
    void close();

public :
    // 获取section-key-value
    bool get( const char * section, const char * key, bool & value );
    bool get( const char * section, const char * key, float & value );
    bool get( const char * section, const char * key, int8_t & value );
    bool get( const char * section, const char * key, uint8_t & value );
    bool get( const char * section, const char * key, int16_t & value );
    bool get( const char * section, const char * key, uint16_t & value );
    bool get( const char * section, const char * key, int32_t & value );
    bool get( const char * section, const char * key, uint32_t & value );
    bool get( const char * section, const char * key, int64_t & value );
    bool get( const char * section, const char * key, uint64_t & value );
    bool get( const char * section, const char * key, std::string & value );

private :
    // 解析配置文件
    void parse( char * filecontent, int32_t filesize );

    // 获取指定section指定key的value值
    const char * getValue( const char * section, const char * key );

private :
    typedef std::map<std::string, ConfigSection *> SectionMap;

    std::string     m_File;
    SectionMap      m_Sections;
};

//
// 日志文件
//
class Logger;
class CShmem;
class CSemlock;

class LogFile
{
public :
    // 日志级别
    enum
    {
        eLogLevel_Fatal = 1,        // 严重
        eLogLevel_Error = 2,        // 错误
        eLogLevel_Warn  = 3,        // 警告
        eLogLevel_Info  = 4,        // 信息
        eLogLevel_Trace = 5,        // 跟踪
        eLogLevel_Debug = 6,        // 调试
    };

    LogFile( const char * path, const char * module );
    ~LogFile();

public :
    // 打开日志文件
    bool open();

    // 打印日志
    // 将format中的内容直接输出到日志中
    void print( uint8_t level, const char * format, ... );

    // 打印日志
    // 定前缀+format中的内容一并输出到日志中
    // %t - 年月日 时分秒
    // %T - 年月日 时分秒,毫秒
    // %l %L - 日志等级
    void printp( uint8_t level, const char * prefix, const char * format, ... );

    // 强制刷新日志文件
    // 为了保证数据刷新到文件中, 所以该接口效率比较低
    void flush();

    // 关闭日志文件
    void close();

    // 日志的相关设置
    void setLevel( uint8_t level );
    void setMaxSize( size_t size );

private :
    friend class Logger;

    // 打印日志
    void print( uint8_t level, int32_t today,
            const std::string & head, const std::string & body );

    // 确保key文件存在
    void ensure_keyfiles_exist( char * file1, char * file2 );

    CShmem * getBlock() const { return m_Block; }
    const char * getPath() const { return m_Path.c_str(); }
    const char * getModule() const { return m_Module.c_str(); }

private :
    std::string     m_Path;
    std::string     m_Module;
    CShmem *        m_Block;    // 共享内存块
    CSemlock *      m_Lock;     // 锁
    Logger *        m_Logger;   // 日志
};

}

#endif
