
#ifndef __SRC_UTILS_RANDOM_H__
#define __SRC_UTILS_RANDOM_H__

#include <stdint.h>

namespace Utils
{

class MTRand32
{
public :
    MTRand32( uint32_t seed = 5489UL );
    ~MTRand32();

public :
    uint32_t rand();

private :
    enum
    {
        N           = 624,
        M           = 397,
        MATRIX      = 0x9908b0dfUL,
        UPPER_MASK  = 0x80000000UL,
        LOWER_MASK  = 0x7fffffffUL,
    };

    uint32_t    m_mti;
    uint32_t    m_mt[N];
};

class MTRand64
{
public :
    MTRand64( uint64_t seed = 5489ULL );
    ~MTRand64();

public :
    uint64_t rand();

private :
    enum
    {
        N           = 312,
        M           = 156,
        MATRIX      = 0xB5026F5AA96619E9ULL,
        UPPER_MASK  = 0xFFFFFFFF80000000ULL,
        LOWER_MASK  = 0x7FFFFFFFULL,
    };

    uint32_t    m_mti;
    uint64_t    m_mt[N];
};

// 默认采用MTRand32随机数生成器
class Random : public MTRand32
{
public :
    Random() : MTRand32() {}
    Random( uint32_t seed ) : MTRand32(seed) {}

    ~Random(){}
};

class RandomDevice
{
public :
    RandomDevice();
    ~RandomDevice();

    uint32_t get();

private :
    int32_t m_File;
};

}

#endif
