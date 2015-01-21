
#include <cstdlib>
#include <cstring>

#include "streambuf.h"

//
#define bswap16(_x)        (uint16_t)((_x) << 8 | (_x) >> 8)
#define bswap32(_x)                     \
    (((_x) >> 24) |                     \
    (((_x) & (0xff << 16)) >> 8) |      \
    (((_x) & (0xff << 8)) << 8) |       \
    ((_x) << 24))
#define bswap64(_x)                     \
    (((_x) >> 56) |                     \
    (((_x) >> 40) & (0xffUL << 8)) |    \
    (((_x) >> 24) & (0xffUL << 16)) |   \
    (((_x) >> 8) & (0xffUL << 24)) |    \
    (((_x) << 8) & (0xffUL << 32)) |    \
    (((_x) << 24) & (0xffUL << 40)) |   \
    (((_x) << 40) & (0xffUL << 48)) |   \
    ((_x) << 56))

#if ( 1>>1 == 0 )
    // Little Endian
    #ifndef htobe16
        #define    htobe16(x)    bswap16((uint16_t)(x))
    #endif
    #ifndef htobe32
        #define    htobe32(x)    bswap32((uint32_t)(x))
    #endif
    #ifndef htobe64
        #define    htobe64(x)    bswap64((uint64_t)(x))
    #endif
    #ifndef htole16
        #define    htole16(x)    ((uint16_t)(x))
    #endif
    #ifndef htole32
        #define    htole32(x)    ((uint32_t)(x))
    #endif
    #ifndef htole64
        #define    htole64(x)    ((uint64_t)(x))
    #endif
    #ifndef be16toh
        #define    be16toh(x)    bswap16((uint16_t)(x))
    #endif
    #ifndef be32toh
        #define    be32toh(x)    bswap32((uint32_t)(x))
    #endif
    #ifndef be64toh
        #define    be64toh(x)    bswap64((uint64_t)(x))
    #endif
    #ifndef le16toh
        #define    le16toh(x)    ((uint16_t)(x))
    #endif
    #ifndef le32toh
        #define    le32toh(x)    ((uint32_t)(x))
    #endif
    #ifndef le64toh
        #define    le64toh(x)    ((uint64_t)(x))
    #endif
#else
    // Big Endian
    #ifndef htobe16
        #define    htobe16(x)    ((uint16_t)(x))
    #endif
    #ifndef htobe32
        #define    htobe32(x)    ((uint32_t)(x))
    #endif
    #ifndef htobe64
        #define    htobe64(x)    ((uint64_t)(x))
    #endif
    #ifndef htole16
        #define    htole16(x)    bswap16((uint16_t)(x))
    #endif
    #ifndef htole32
        #define    htole32(x)    bswap32((uint32_t)(x))
    #endif
    #ifndef htole64
        #define    htole64(x)    bswap64((uint64_t)(x))
    #endif
    #ifndef be16toh
        #define    be16toh(x)    ((uint16_t)(x))
    #endif
    #ifndef be32toh
        #define    be32toh(x)    ((uint32_t)(x))
    #endif
    #ifndef be64toh
        #define    be64toh(x)    ((uint64_t)(x))
    #endif
    #ifndef le16toh
        #define    le16toh(x)    bswap16((uint16_t)(x))
    #endif
    #ifndef le32toh
        #define    le32toh(x)    bswap32((uint32_t)(x))
    #endif
    #ifndef le64toh
        #define    le64toh(x)    bswap64((uint64_t)(x))
    #endif
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

StreamBuf::StreamBuf( uint32_t offset, int8_t endian )
    : m_Endian( endian ),
      m_Method( eMethod_Encode ),
      m_Buffer( NULL ),
      m_Length( 0 ),
      m_Offset( offset ),
      m_IsExpand( true )
{}

StreamBuf::StreamBuf( int8_t method, char * buf, uint32_t len, int8_t endian )
    : m_Endian( endian ),
      m_Method( method ),
      m_Buffer( buf ),
      m_Length( len ),
      m_Offset( 0 ),
      m_IsExpand( false )
{}

StreamBuf::~StreamBuf()
{
    if ( m_IsExpand && m_Buffer )
    {
        std::free( m_Buffer );
        m_Buffer = NULL;
    }
}

bool StreamBuf::expand( uint32_t len )
{
    uint32_t size = len + m_Offset;

    if ( size > m_Length )
    {
        if ( !m_IsExpand )
        {
            return false;
        }

        uint32_t newlength = 8;
        while ( newlength < size )
        {
            newlength <<= 1;
        }

        char * newbuffer = (char *)std::realloc(m_Buffer, newlength);
        if ( newbuffer == NULL )
        {
            return false;
        }

        m_Buffer = newbuffer;
        m_Length = newlength;
    }

    return true;
}

bool StreamBuf::code( bool & data )
{
    int8_t d = 0;
    bool rc = false;

    if ( m_Method == eMethod_Encode )
    {
        d = data ? 1 : 0;
        rc = code( d );
    }
    else
    {
        rc = code( d );
        data = (d == 1);
    }
    return rc;
}

bool StreamBuf::code( int8_t & data )
{
    uint32_t len = sizeof(int8_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(int8_t *)(m_Buffer+m_Offset) = data;
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(int8_t *)( m_Buffer+m_Offset );
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( uint8_t & data )
{
    uint32_t len = sizeof(uint8_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(uint8_t *)(m_Buffer+m_Offset) = data;
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(uint8_t *)( m_Buffer+m_Offset );
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( int16_t & data )
{
    uint32_t len = sizeof(int16_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(int16_t *)(m_Buffer+m_Offset)
                = (m_Endian == eEndian_Big) ? htobe16(data) : htole16(data);
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(int16_t *)( m_Buffer+m_Offset );
            data = (m_Endian == eEndian_Big) ? be16toh(data) : le16toh(data);
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( uint16_t & data )
{
    uint32_t len = sizeof(uint16_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(uint16_t *)(m_Buffer+m_Offset)
                = (m_Endian == eEndian_Big) ? htobe16(data) : htole16(data);
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(uint16_t *)( m_Buffer+m_Offset );
            data = (m_Endian == eEndian_Big) ? be16toh(data) : le16toh(data);
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( int32_t & data )
{
    uint32_t len = sizeof(int32_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(int32_t *)(m_Buffer+m_Offset)
                = (m_Endian == eEndian_Big) ? htobe32(data) : htole32(data);
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(int32_t *)( m_Buffer+m_Offset );
            data = (m_Endian == eEndian_Big) ? be32toh(data) : le32toh(data);
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( uint32_t & data )
{
    uint32_t len = sizeof(uint32_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(uint32_t *)(m_Buffer+m_Offset)
                = (m_Endian == eEndian_Big) ? htobe32(data) : htole32(data);
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(uint32_t *)( m_Buffer+m_Offset );
            data = (m_Endian == eEndian_Big) ? be32toh(data) : le32toh(data);
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( int64_t & data )
{
    uint32_t len = sizeof(int64_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(int64_t *)(m_Buffer+m_Offset)
                = (m_Endian == eEndian_Big) ? htobe64(data) : htole64(data);
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(int64_t *)( m_Buffer+m_Offset );
            data = (m_Endian == eEndian_Big) ? be64toh(data) : le64toh(data);
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( uint64_t & data )
{
    uint32_t len = sizeof(uint64_t);

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        if ( expand(len) )
        {
            *(uint64_t *)(m_Buffer+m_Offset)
                = (m_Endian == eEndian_Big) ? htobe64(data) : htole64(data);
            m_Offset += len;
            return true;
        }
    }
    else
    {
        // decoding
        if ( m_Offset + len <= m_Length )
        {
            data = *(uint64_t *)( m_Buffer+m_Offset );
            data = (m_Endian == eEndian_Big) ? be64toh(data) : le64toh(data);
            m_Offset += len;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( std::string & data )
{
    uint32_t length = 0;

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        length = data.size() + 1;

        if ( code(length) )
        {
            if ( expand(length) )
            {
                std::memcpy( m_Buffer+m_Offset, data.c_str(), length );
                m_Offset += length;
                return true;
            }
        }
    }
    else
    {
        // decoding
        if ( code( length ) )
        {
            data.resize( length-1 );
            data.assign( m_Buffer+m_Offset, length-1 );
            m_Offset += length;
            return true;
        }
    }

    return false;
}

bool StreamBuf::code( char * data, uint16_t & len )
{
    uint16_t length = 0;

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        length = len+1;
        data = ( data == NULL ? (char*)"" : data );

        if ( code(length) )
        {
            if ( expand(length) )
            {
                std::memcpy( m_Buffer+m_Offset, data, length );
                m_Offset += length;
                return true;
            }
        }
    }
    else
    {
        // decoding
        if ( code(length) )
        {
            if ( m_Offset+length <= m_Length
                    && m_Buffer[m_Offset+length-1] == 0 )
            {
                std::memcpy( data, m_Buffer+m_Offset, length );
                m_Offset += length;
                len = length - 1;
                return true;
            }
        }

        len = 0;
        data[len] = 0;
    }

    return false;
}

bool StreamBuf::code( char * data, uint32_t & len )
{
    uint32_t length = 0;

    if ( m_Method == eMethod_Encode )
    {
        // encoding
        length = len+1;
        data = ( data == NULL ? (char*)"" : data );

        if ( code(length) )
        {
            if ( expand(length) )
            {
                std::memcpy( m_Buffer+m_Offset, data, length );
                m_Offset += length;
                return true;
            }
        }
    }
    else
    {
        // decoding
        if ( code(length) )
        {
            if ( m_Offset+length <= m_Length
                    && m_Buffer[m_Offset+length-1] == 0 )
            {
                std::memcpy( data, m_Buffer+m_Offset, length );
                m_Offset += length;
                len = length - 1;
                return true;
            }
        }

        len = 0;
        data[len] = 0;
    }

    return false;
}

char * StreamBuf::data() const
{
    if ( m_Method == eMethod_Encode )
    {
        return m_Buffer;
    }

    return NULL;
}

uint32_t StreamBuf::length() const
{
    if ( m_Method == eMethod_Encode )
    {
        return m_Offset;
    }

    return 0;
}

void StreamBuf::clear()
{
    m_Buffer = NULL;
    m_Length = 0;
    m_Offset = 0;
    m_IsExpand = true;
}

void StreamBuf::reset()
{
    m_Offset = 0;
}
