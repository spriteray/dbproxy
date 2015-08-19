
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hashfunc.h"
#include "ipcs.h"

// linux必须显示的声明semun
#if defined (__linux__)
union semun
{
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};
#endif

namespace Utils
{

CSemlock::CSemlock( const char * keyfile )
    : m_IsOwner(false),
      m_SemId(-1),
      m_Keyfile(keyfile)
{}

CSemlock::~CSemlock()
{
    m_IsOwner = false;
    m_SemId = -1;
    m_Keyfile.clear();
}

bool CSemlock::init()
{
    key_t semkey = 0;

    m_IsOwner = true;

    if ( !m_Keyfile.empty() )
    {
        int32_t projectid = (int32_t)HashFunction::djb( m_Keyfile.c_str(), m_Keyfile.length() );

        semkey = ::ftok( const_cast<char*>(m_Keyfile.c_str()), projectid );
        if ( semkey == -1 )
        {
            return false;
        }

        m_SemId = ::semget( semkey, 1, IPC_CREAT|IPC_EXCL|0600 );
        if ( m_SemId < 0 )
        {
            if ( errno != EEXIST )
            {
                return false;
            }

            m_SemId = ::semget( semkey, 0, 0600 );
            if ( m_SemId < 0 )
            {
                return false;
            }

            m_IsOwner = false;
        }
    }
    else
    {
        m_SemId = ::semget( IPC_PRIVATE, 1, IPC_CREAT|0600 );
        if ( m_SemId < 0 )
        {
            return false;
        }
    }

    if ( m_IsOwner )
    {
        union semun ick;
        ick.val = 1;

        if ( ::semctl(m_SemId, 0, SETVAL, ick) < 0 )
        {
            return false;
        }
    }

    return true;
}

void CSemlock::final()
{
    if ( m_IsOwner && m_SemId != -1 )
    {
        union semun ick;
        ick.val = 0;
        ::semctl( m_SemId, 0, IPC_RMID, ick );

        m_SemId = -1;
        m_IsOwner = false;
    }

    return;
}

void CSemlock::lock()
{
    int32_t rc = -1;
    struct sembuf sem_on;

    sem_on.sem_num     = 0;
    sem_on.sem_op    = -1;
    sem_on.sem_flg    = SEM_UNDO;

    do
    {
        rc = ::semop( m_SemId, &sem_on, 1 );
    } while ( rc < 0 && errno == EINTR );

    if ( rc < 0 && (errno == EIDRM || errno ==EINVAL) )
    {
        // 信号灯被删除了
        // 清空信号灯, 再次初始化
        // 重新加锁

        final();
        init();

        lock();
    }

    return;
}

void CSemlock::unlock()
{
    int32_t rc = -1;
    struct sembuf sem_off;

    sem_off.sem_num     = 0;
    sem_off.sem_op      = 1;
    sem_off.sem_flg     = SEM_UNDO;

    do
    {
        rc = ::semop( m_SemId, &sem_off, 1 );
    } while ( rc < 0 && errno == EINTR );

    if ( rc < 0 && (errno == EIDRM || errno ==EINVAL) )
    {
        // 信号灯被删除了
        // 清空信号灯, 再次初始化

        final();
        init();
    }

    return;
}

bool CSemlock::isOwner() const
{
    return m_IsOwner;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CShmem::CShmem( const char * keyfile )
    : m_IsOwner(true),
      m_ShmId(-1),
      m_Keyfile(keyfile)
{}

CShmem::~CShmem()
{}

bool CShmem::alloc( size_t size )
{
    key_t shmkey = -1;
    int32_t projectid = 0;

    if ( m_Keyfile.empty() )
    {
        m_ShmId = ::shmget( IPC_PRIVATE, size, IPC_CREAT );
        goto ALLOCATE_RESULT;
    }

    projectid = (int32_t)HashFunction::djb( m_Keyfile.c_str(), m_Keyfile.length() );
    shmkey = ::ftok( const_cast<char *>(m_Keyfile.c_str()), projectid );
    if ( shmkey == -1 )
    {
        goto ALLOCATE_RESULT;
    }

    m_ShmId = ::shmget( shmkey, size, IPC_CREAT|IPC_EXCL|0600 );
    if ( m_ShmId < 0 && errno != EEXIST )
    {
        goto ALLOCATE_RESULT;
    }

    if ( m_ShmId < 0 )
    {
        m_IsOwner = false;
        m_ShmId = ::shmget( shmkey, size, 0600 );
    }

ALLOCATE_RESULT :

    if ( m_ShmId < 0 )
    {
        m_IsOwner = false;
        m_ShmId = -1;
        return false;
    }

    return true;
}

void CShmem::free()
{
    if ( m_ShmId >= 0 )
    {
        if ( m_IsOwner )
        {
            ::shmctl( m_ShmId, IPC_RMID, 0 );
        }
        m_ShmId = -1;
    }
}

void * CShmem::link()
{
    if ( m_ShmId >= 0 )
    {
        return ::shmat( m_ShmId, (const void *)0, 0 );
    }

    return NULL;
}

void CShmem::unlink( void * ptr )
{
    if ( m_ShmId >= 0 && ptr != NULL )
    {
        ::shmdt(ptr);
    }
}

bool CShmem::isOwner() const
{
    return m_IsOwner;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CMmapAllocator::CMmapAllocator( const char * keyfile )
    : m_IsOwner( false ),
      m_Size( 0ULL ),
      m_Keyfile( keyfile )
{}

CMmapAllocator::~CMmapAllocator()
{}

void * CMmapAllocator::alloc( size_t size )
{
    int32_t fd = ::open( m_Keyfile.c_str(), O_RDWR );
    if ( fd < 0 )
    {
        // 没有权限
        if ( errno != ENOENT )
        {
            return NULL;
        }

        // 文件不存在, 新建文件
        fd = ::open( m_Keyfile.c_str(), O_RDWR|O_CREAT, 0666 );
        if ( fd < 0 )
        {
            return NULL;
        }

        // 默认填充
        m_IsOwner = true;
        this->padding( fd, 0, size );
    }

    // 确保文件足够大
    if ( !this->ensureSpace( fd, size ) )
    {
        ::close( fd );
        return NULL;
    }

    // 映射
    void * result = NULL;
    result = ::mmap( 0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0 );
    if ( result == MAP_FAILED )
    {
        ::close( fd );
        return NULL;
    }

    // 关闭文件
    ::close( fd );
    m_Size = size;

    return result;
}

void CMmapAllocator::free( void * ptr )
{
    ::munmap( ptr, m_Size );
}

bool CMmapAllocator::ensureSpace( int32_t fd, size_t size )
{
    struct stat filestat;

    // 判断文件大小
    if ( ::fstat( fd, &filestat ) != 0 )
    {
        return false;
    }

    if ( (size_t)filestat.st_size > size )
    {
        return false;
    }

    // 扩充
    this->padding( fd, filestat.st_size, size );
    return true;
}

void CMmapAllocator::padding( int32_t fd, size_t offset, size_t size )
{
    char buffer[ 1024 ] = { 0 };

    // 默认填充
    for ( size_t i = offset; i < size; i += sizeof(buffer) )
    {
        ::write( fd, buffer, sizeof(buffer) );
    }

    // 重置
    ::ftruncate( fd, size );
    ::lseek( fd, 0, SEEK_SET );
}

}
