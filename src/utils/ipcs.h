
#ifndef __SRC_UTILS_IPCS_H__
#define __SRC_UTILS_IPCS_H__

#include <string>

#include <stdint.h>
#include <unistd.h>

namespace Utils
{

class CSemlock
{
public :
    CSemlock( const char * keyfile );
    ~CSemlock();

public :
    bool init();
    void final();

    void lock();
    void unlock();

    bool isOwner() const;

private :
    bool            m_IsOwner;
    int32_t         m_SemId;
    std::string     m_Keyfile;
};

class CShmem
{
public :
    CShmem( const char * keyfile );
    ~CShmem();

public :
    bool alloc( size_t size );
    void free();

    void * link();
    void unlink( void * ptr );

    bool isOwner() const;

private :
    bool            m_IsOwner;
    int32_t         m_ShmId;
    std::string     m_Keyfile;
};

class CMmapAllocator
{
public :
    CMmapAllocator( const char * keyfile );
    ~CMmapAllocator();

public :
    void * alloc( size_t size );
    void free( void * ptr );

    bool isOwner() const { return m_IsOwner; }

private :
    bool ensureSpace( int32_t fd, size_t size );
    void padding( int32_t fd, size_t offset, size_t size );

private :
    bool            m_IsOwner;
    size_t          m_Size;
    std::string     m_Keyfile;
};

}

#endif
