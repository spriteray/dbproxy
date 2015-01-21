
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__linux__)
#include <linux/limits.h>
#else
#include <limits.h>
#endif

#include "ipcs.h"
#include "file.h"

namespace Utils
{

//
// 日志缓存
//
struct Buffer
{
    int32_t     date;       // 记录日志的日期
    int32_t     index;      // 记录当前日期下日志文件的索引
    uint8_t     minlevel;   //
    size_t      sizelimit;  // 文件大小限制
    int32_t     offsets;    // 日志缓存的偏移量
    char        buffer[0];  // 日志缓存

    Buffer()
    {
        date        = 0;
        index       = 0;
        minlevel    = 0;
        sizelimit   = 0;
        offsets     = 0;
    };
};

//
// 日志记录器
//
class Logger
{
public :
    Logger( LogFile * file );
    ~Logger();

    static int32_t getToday();
    static size_t getBlockSize();

public :
    // 初始化
    void initialize( int32_t today = 0, bool isinit = true );

    // 获取日志文件大小
    ssize_t getFileSize();

    // 获取日志文件路径
    void getLogPath( char * path );
    void getIndexPath( char * path );

    // 设置/获取今天的日期
    void setDate( int32_t today );
    int32_t getDate() const { return m_Buffer->date; }

    // 获取/设置日志等级
    void setLevel( uint8_t level );
    uint8_t getLevel() const { return m_Buffer->minlevel; }

    // 获取偏移量
    int32_t getOffset() const { return m_Buffer->offsets; }

    // 设置日志文件限制
    void setSizeLimit( size_t size ) { m_Buffer->sizelimit = size; }

public :
    void open();
    void flush();
    void append( const std::string & logline );
    void rotate();

private :
    LogFile *   m_LogFile;

    int32_t     m_Fd;
    size_t      m_Size;
    Buffer *    m_Buffer;
};

Logger::Logger( LogFile * file )
    : m_LogFile( file ),
      m_Fd( -1 ),
      m_Size( 0 ),
      m_Buffer( NULL )
{}

Logger::~Logger()
{
    // 刷新日志到文件中
    this->flush();

    // 关闭文件
    if ( m_Fd != -1 )
    {
        ::close( m_Fd );
        m_Fd = -1;
    }

    if ( m_Buffer != NULL )
    {
        m_LogFile->getBlock()->unlink( m_Buffer );
        m_Buffer = NULL;
    }
}

void Logger::initialize( int32_t today, bool isinit )
{
    // 连接内存块
    m_Buffer = (Buffer *)m_LogFile->getBlock()->link();
    assert( m_Buffer != NULL && "Logger link ShareMemory failed" );

    // 新建内存块的时候才会初始化
    // 隔天或者交换日志的时候不需要初始化
    if ( isinit && m_LogFile->getBlock()->isOwner() )
    {
        m_Buffer->offsets   = 0;
        m_Buffer->sizelimit = 0;
        m_Buffer->minlevel  = 0;
    }

    if ( today == 0 )
    {
        today = Logger::getToday();
    }

    // 设置今天的日期
    this->setDate( today );
    m_Size = Logger::getBlockSize();

    // 是否需要打开文件
    this->open();
}

int32_t Logger::getToday()
{
    struct tm tm_now;
    time_t now = time(NULL);
    localtime_r( &now, &tm_now );

    return (tm_now.tm_year+1900)*10000+(tm_now.tm_mon+1)*100+tm_now.tm_mday;
}

size_t Logger::getBlockSize()
{
    // 获取块大小
    struct stat fs;
    stat("/dev/null", &fs);
    return fs.st_blksize;
}

ssize_t Logger::getFileSize()
{
    char path[PATH_MAX];
    struct stat filestat;

    getLogPath( path );
    if ( ::stat(path, &filestat) == 0 )
    {
        return filestat.st_size;
    }

    return -1;
}

void Logger::getLogPath( char * path )
{
    std::snprintf( path, PATH_MAX,
            "%s/%s-%d.log",
            m_LogFile->getPath(),
            m_LogFile->getModule(), m_Buffer->date );
}

void Logger::getIndexPath( char * path )
{
    std::snprintf( path, PATH_MAX,
            "%s/%s-%d.%d.log",
            m_LogFile->getPath(),
            m_LogFile->getModule(),
            m_Buffer->date, m_Buffer->index );
}

void Logger::setDate( int32_t today )
{
    m_Buffer->index = 1;
    m_Buffer->date  = today;
}

void Logger::setLevel( uint8_t level )
{
    if ( level <= LogFile::eLogLevel_Debug )
    {
        m_Buffer->minlevel = level;
    }
}

void Logger::open()
{
    char path[PATH_MAX];

    // 拼接文件名
    getLogPath( path );

    // 打开文件
    m_Fd = ::open( path, O_RDWR|O_APPEND|O_CREAT, 0644 );
    assert( m_Fd != -1 && "Logger::open() LogFile failed ." );
}

void Logger::append( const std::string & logline )
{
    size_t size = logline.size();
    char * data = const_cast<char *>( logline.c_str() );

    // 日志缓冲区空间不足的情况下, 需要将日志数据落地到文件中
    if ( m_Buffer->offsets + size >= m_Size )
    {
        this->flush();
    }

    if ( size >= m_Size )
    {
        // 太长的日志行不进行缓冲直接写日志文件
        ::write( m_Fd, data, size );
        ::fsync( m_Fd );
    }
    else
    {
        // 记录新的日志信息
        std::memcpy( m_Buffer->buffer+m_Buffer->offsets, data, size );
        m_Buffer->offsets += size;
    }
}

void Logger::flush()
{
    if ( getOffset() == 0 )
    {
        // 日志已经全部刷新到日志文件中
        return;
    }

    // 为了防止日志文件不完整,
    // 每次flush都会把缓冲区的数据刷新到当前日志文件中

    // 日志文件正常, 记录日志文件的尺寸
    ssize_t filesize = getFileSize();
    if ( filesize == -1 )
    {
        // 文件不存在
        ::close( m_Fd );
        this->open();
        filesize = 0;
    }

    filesize += m_Buffer->offsets;

    // 把日志缓冲区的日志落地到已经存在的日志文件中去
    ::write( m_Fd, m_Buffer->buffer, m_Buffer->offsets );
    ::fsync( m_Fd );
    m_Buffer->offsets = 0;

    // 只有flush的情况下, 才会引发文件大小的改变
    // 此时去进行日志尺寸的检查最为合适了
    if ( m_Buffer->sizelimit != 0
            && filesize > (ssize_t)m_Buffer->sizelimit )
    {
        // 当前的日志文件已经被删除或者文件尺寸过大
        this->rotate();
    }
}

void Logger::rotate()
{
    char oldfile[PATH_MAX];
    char newfile[PATH_MAX];

    getLogPath( oldfile );
    getIndexPath( newfile );

    // 关闭文件
    ::close( m_Fd );

    // 重命名文件, 以及增加文件索引号
    ++m_Buffer->index;
    std::rename( oldfile, newfile );

    // 打开文件
    this->open();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

LogFile::LogFile( const char * path, const char * module )
    : m_Path(path),
      m_Module(module),
      m_Block( NULL ),
      m_Lock( NULL ),
      m_Logger( NULL )
{}

LogFile::~LogFile()
{}

bool LogFile::open()
{
    // 初始化锁和共享内存的key文件
    char lock_keyfile[PATH_MAX] = {0};
    char shmem_keyfile[PATH_MAX] = {0};

    // 确保文件存在
    ensure_keyfiles_exist( lock_keyfile, shmem_keyfile );

    // 初始化缓冲区锁
    m_Lock = new CSemlock( lock_keyfile );
    if ( !m_Lock->init() )
    {
        delete m_Lock;
        return false;
    }

    // 初始化日志缓冲区
    m_Block = new CShmem( shmem_keyfile );
    if ( !m_Block->alloc(Logger::getBlockSize()+sizeof(Buffer)) )
    {
        // 日志缓冲区分配失败
        delete m_Block;
        m_Lock->final();
        delete m_Lock;
        return false;
    }

    m_Lock->lock();

    // 初始化日志文件
    m_Logger = new Logger( this );
    assert( m_Logger != NULL && "create Logger failed" );
    m_Logger->initialize();
    if ( m_Logger->getOffset() > 0 ) m_Logger->flush();

    m_Lock->unlock();

    return true;
}

void LogFile::print( uint8_t level, const char * format, ... )
{
    // 过滤非法日志等级
    if ( level > eLogLevel_Debug )
    {
        return;
    }

    // 日志等级的判断
    if ( m_Logger->getLevel() != 0
            && level > m_Logger->getLevel() )
    {
        // 不加锁对最低的日志等级的判断,
        // 有可能引发的顺序问题可以不做考虑
        // 最坏的情况就是
        // 改变日志等级一瞬间的部分日志信息没有记录下来, 这很严重吗?
        return;
    }

    // 日志数据
    int32_t ndata = 0;
    char * data = NULL;

    va_list args;
    va_start( args, format );
    ndata = vasprintf( &data, format, args );
    va_end( args );

    // 日志行
    int32_t today = 0;
    std::string logline( ndata+128, 0 );
    spliceLogline( level,
            today, const_cast<char *>(data), logline );
    std::free( data );

    // 加锁写日志
    m_Lock->lock();

    // 日志文件是否隔天了
    if ( today != m_Logger->getDate() )
    {
        // 创建当天的日志记录器
        delete m_Logger;
        m_Logger = new Logger( this );
        assert( m_Logger != NULL && "create Logger failed" );
        m_Logger->initialize( today, false );
    }

    // 写日志
    // 轮换日志文件在append()中发生
    m_Logger->append( logline );

    // 解锁
    m_Lock->unlock();
}

void LogFile::flush()
{
    m_Lock->lock();
    m_Logger->flush();
    m_Lock->unlock();
}

void LogFile::setLevel( uint8_t level )
{
    m_Lock->lock();
    m_Logger->setLevel( level );
    m_Lock->unlock();
}

void LogFile::setMaxSize( size_t size )
{
    m_Lock->lock();
    m_Logger->setSizeLimit( size );
    m_Lock->unlock();
}

void LogFile::close()
{
    if ( m_Lock != NULL )
    {
        m_Lock->lock();
    }

    if ( m_Logger != NULL )
    {
        delete m_Logger;
        m_Logger = NULL;
    }

    if ( m_Block != NULL )
    {
        // 销毁内存块
        m_Block->free();
        delete m_Block;
        m_Block = NULL;
    }

    if ( m_Lock != NULL )
    {
        // 销毁锁
        m_Lock->unlock();
        m_Lock->final();
        delete m_Lock;
        m_Lock = NULL;
    }
}

void LogFile::ensure_keyfiles_exist( char * file1, char * file2 )
{
    std::snprintf( file1, PATH_MAX,
            "%s/.log_%s_lock.key", m_Path.c_str(), m_Module.c_str() );
    std::snprintf( file2, PATH_MAX,
            "%s/.log_%s_shmem.key", m_Path.c_str(), m_Module.c_str() );

    // 确保文件存在
    ::close ( ::open(file1, O_WRONLY|O_CREAT, 0644) );
    ::close ( ::open(file2, O_WRONLY|O_CREAT, 0644) );
}

// 拼接日志行
// 鉴于localtime_r()非常低效,
// 所以还是增加了today这个从设计上来说很ugly的参数
void LogFile::spliceLogline( uint8_t level,
        int32_t & today, const char * data, std::string & logline )
{
    const char *loglevel_desc[] =
    {
        "Fatal", "Error", "Warn",
        "Info", "Trace", "Debug"
    };

    // 前缀
    char * prefix = (char *)( logline.c_str() );

    // 时间
    struct tm tm_now;
    time_t now = time(NULL);
    localtime_r( &now, &tm_now );

    // 计算今天的日期
    today = (tm_now.tm_year+1900)*10000+(tm_now.tm_mon+1)*100+tm_now.tm_mday;

    // 日志前缀
    int32_t nprefix = strftime( prefix, 128, "%Y-%m-%d %H:%M:%S ", &tm_now );
    if ( level == 0 )
    {
        std::strcat( prefix, ": " );
        nprefix += 2;
    }
    else
    {
        nprefix += std::sprintf( prefix+nprefix, "[%s]\t : ", loglevel_desc[level-1] );
    }

    // 设置logline的长度
    logline.resize( nprefix );
    logline += data;
}

}

#define TEST_UNIT    0
#if TEST_UNIT

int main()
{
    int32_t i = 0;

    Utils::LogFile logger( ".", "libudk" );
    if ( !logger.open() )
    {
        printf("open LogFile failed .\n");
        return -1;
    }
    logger.setMaxSize( 5*1024*1024 );

    srand( time(NULL) );
    for ( i = 1; i < 50000; ++i )
    {
        uint8_t level = 1+rand()%5;
        logger.print( level, "test logfile %d .\n", i );
    }

    logger.close();

    return 0;
}

#endif
