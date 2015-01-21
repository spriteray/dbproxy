
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "random.h"

namespace Utils
{

MTRand32::MTRand32( uint32_t seed )
{
    m_mt[0] = seed & 0xffffffffUL;
    for ( m_mti = 1; m_mti < N; ++m_mti )
    {
        m_mt[m_mti] = (1812433253UL * (m_mt[m_mti-1] ^ (m_mt[m_mti-1] >> 30)) + m_mti);
        m_mt[m_mti] &= 0xffffffffUL;
    }
}

uint32_t MTRand32::rand()
{
    uint32_t i;
    uint32_t x;
    static uint32_t magic[2] = { 0x0UL, MATRIX };

    if ( m_mti >= N )
    {
        /* generate NN words at one time */
        for ( i = 0; i < N-M; ++i )
        {
            x = (m_mt[i]&UPPER_MASK)|(m_mt[i+1]&LOWER_MASK);
            m_mt[i] = m_mt[i+M] ^ (x >> 1) ^ magic[x & 0x1UL];
        }
        for ( ; i < N-1; ++i )
        {
            x = (m_mt[i]&UPPER_MASK)|(m_mt[i+1]&LOWER_MASK);
            m_mt[i] = m_mt[i+(M-N)] ^ (x >> 1) ^ magic[x & 0x1UL];
        }

        x = (m_mt[N-1]&UPPER_MASK)|(m_mt[0]&LOWER_MASK);
        m_mt[N-1] = m_mt[M-1] ^ (x >> 1) ^ magic[x & 0x1UL];

        m_mti = 0;
    }

    x = m_mt[m_mti++];

    /* Tempering */
    x ^= (x >> 11);
    x ^= (x << 7) & 0x9d2c5680UL;
    x ^= (x << 15) & 0xefc60000UL;
    x ^= (x >> 18);

    return x;
}

MTRand32::~MTRand32()
{}

MTRand64::MTRand64( uint64_t seed )
{
    m_mt[0] = seed;

    for ( m_mti = 1; m_mti < N; ++m_mti )
    {
        m_mt[m_mti] = (6364136223846793005ULL * (m_mt[m_mti-1] ^ (m_mt[m_mti-1] >> 62)) + m_mti);
    }
}

uint64_t MTRand64::rand()
{
    uint32_t i;
    uint64_t x;
    static uint64_t magic[2] = { 0ULL, MATRIX };

    if ( m_mti >= N )
    {
        /* generate NN words at one time */
        for ( i = 0; i < N-M; ++i )
        {
            x = (m_mt[i]&UPPER_MASK)|(m_mt[i+1]&LOWER_MASK);
            m_mt[i] = m_mt[i+M] ^ (x>>1) ^ magic[(int32_t)(x&1ULL)];
        }
        for ( ; i < N-1; ++i )
        {
            x = (m_mt[i]&UPPER_MASK)|(m_mt[i+1]&LOWER_MASK);
            m_mt[i] = m_mt[i+(M-N)] ^ (x>>1) ^ magic[(int32_t)(x&1ULL)];
        }

        x = (m_mt[N-1]&UPPER_MASK)|(m_mt[0]&LOWER_MASK);
        m_mt[N-1] = m_mt[M-1] ^ (x>>1) ^ magic[(int32_t)(x&1ULL)];

        m_mti = 0;
    }

    x = m_mt[m_mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

MTRand64::~MTRand64()
{}

RandomDevice::RandomDevice()
{
    m_File = ::open( "/dev/urandom", O_RDONLY);
}

RandomDevice::~RandomDevice()
{
    ::close( m_File );
    m_File = -1;
}

uint32_t RandomDevice::get()
{
    uint32_t value = 0;
    read( m_File, &value, sizeof(value) );
    return value;
}

}
