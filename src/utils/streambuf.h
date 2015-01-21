
#ifndef __SRC_UTILS_STREAMBUFFER_H__
#define __SRC_UTILS_STREAMBUFFER_H__

#include <vector>
#include <string>

#include <stdint.h>

class StreamBuf
{
public :
    enum
    {
        eMethod_Encode    = 1,
        eMethod_Decode    = 2,
        eEndian_Big        = 1,    // 字节序为大端
        eEndian_Little    = 2,    // 字节序为小端
    };

    // Encode StreamBuf
    // offset: 初始化偏移量, 一般是偏移掉包头
    StreamBuf( uint32_t offset=0, int8_t endian=eEndian_Big );

    // 指定缓冲区的StreamBuf
    StreamBuf( int8_t method, char * buf, uint32_t len, int8_t endian=eEndian_Big );

    //
    ~StreamBuf();

public :
    bool        code( bool & data );
    bool        code( int8_t & data );
    bool        code( uint8_t & data );
    bool        code( int16_t & data );
    bool        code( uint16_t & data );
    bool        code( int32_t & data );
    bool        code( uint32_t & data );
    bool        code( int64_t & data );
    bool        code( uint64_t & data );
    bool        code( std::string & data );
    bool        code( char * data, uint16_t & len );
    bool        code( char * data, uint32_t & len );

    // 编码vector
    template<class T>
        bool code( std::vector<T> & data )
        {
            if ( m_Method == eMethod_Encode )
            {
                uint16_t count = (uint16_t)data.size();
                if ( !code(count) )
                {
                    return false;
                }

                for ( uint16_t i = 0; i < count; ++i )
                {
                    if ( !code(data[i]) )
                    {
                        return false;
                    }
                }
            }
            else
            {
                uint16_t count = 0;

                if ( !code(count) )
                {
                    return false;
                }

                data.resize( count );

                for ( uint16_t i = 0; i < count; ++i )
                {
                    if ( !code( data[i] ) )
                    {
                        return false;
                    }
                }
            }

            return true;
        }

    char *      data() const;
    uint32_t    length() const;

    // 清空缓冲区, 不会释放内存块
    // 调用者需要显式的释放内存块
    void        clear();

    // 重置
    void        reset();

private :
    // 扩展空间
    bool        expand( uint32_t len );

    int8_t      m_Endian;
    int8_t      m_Method;
    char *      m_Buffer;
    uint32_t    m_Length;
    uint32_t    m_Offset;
    bool        m_IsExpand;
};

#endif
