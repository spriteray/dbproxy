
#include <cassert>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "file.h"

#define TEST_UNIT    0

namespace Utils
{

class ConfigSection
{
public :
    ConfigSection( char * section, int32_t length );
    ~ConfigSection();

public :
    bool addItem( char * key, int32_t nkey, char * value, int32_t nvalue );

    const char * getSection();
    const char * getValue( const char * key );

    static char * trim( char * data, int32_t & len );

private :
    std::string                         m_Section;
    std::map<std::string, std::string>  m_AllItems;
};

ConfigSection::ConfigSection( char * section, int32_t length )
{
    m_AllItems.clear();
    m_Section.assign( section, length );
}

ConfigSection::~ConfigSection()
{
    m_AllItems.clear();
}

bool ConfigSection::addItem( char * key, int32_t nkey, char * value, int32_t nvalue )
{
    if ( nkey <= 0 )
    {
        return false;
    }

    std::string strkey;
    std::string strvalue;

    strkey.assign( key, nkey );
    strvalue.assign( value, nvalue );

    return m_AllItems.insert( std::make_pair( strkey, strvalue ) ).second;
}

const char * ConfigSection::getSection()
{
    return m_Section.c_str();
}

const char * ConfigSection::getValue( const char * key )
{
    std::map<std::string, std::string>::iterator result;

    result = m_AllItems.find( key );
    if ( result != m_AllItems.end() )
    {
        return result->second.c_str();
    }

    return NULL;
}

char * ConfigSection::trim( char * data, int32_t & len )
{
    char * pos = data;
    char * end = pos + len - 1;

    for ( ; (*pos == '\n' || *pos == '\r' || *pos == '\t' || *pos == ' ') && len > 0;  )
    {
        --len;

        if ( len > 0 )  // 确保pos在合法的范围内
        {
            ++pos;
        }
    }

    for ( ; (*end == '\n' || *end == '\r' || *end == '\t' || *end == ' ') && len > 0;  )
    {
        --len;

        if ( len > 0 )  // 确保end在合法的范围内
        {
            --end;
        }
    }

    return pos;
}

// -----------------------------------------------------------------------------------

ConfigFile::ConfigFile( const char * path )
    : m_File( path )
{
    m_Sections.clear();
}

ConfigFile::~ConfigFile()
{}

bool ConfigFile::open()
{
    int32_t fd = ::open( m_File.c_str(), O_RDWR );
    if ( fd < 0 )
    {
        return false;
    }

    int32_t filesize = ::lseek( fd, 0, SEEK_END );
    void * filecontent = ::mmap( 0, filesize, PROT_READ, MAP_SHARED, fd, 0 );

    if ( filecontent == MAP_FAILED )
    {
        ::close( fd );
        return false;
    }

    ::close( fd );
    parse( (char *)filecontent, filesize );
    ::munmap( filecontent, filesize );

    return true;
}

void ConfigFile::close()
{
    // 释放map中的节和项目
    for ( SectionMap::iterator it = m_Sections.begin();
        it != m_Sections.end(); ++it )
    {
        ConfigSection * section = it->second;

        if ( section )
        {
            delete section;
        }
    }
    m_Sections.clear();

    return;
}

void ConfigFile::parse( char * filecontent, int32_t filesize )
{
    char * pos = filecontent;
    ConfigSection * dosection = NULL;

    for( ; pos < filecontent + filesize;  )
    {
        char * endline = std::strchr( pos, '\n' );

        if ( endline == NULL )
        {
            break;
        }

        // 读取配置文件的一行
        char * line = pos;
        int32_t nline = endline - pos;

        // 去除每行开始的空格或者tab
        while ( line[0] == ' ' || line[0] == '\t' )
        {
            ++line;
            --nline;
        }

        if ( line[0] == '#'
            || (dosection == NULL && line[0] != '[') )
        {
            // 注释行, 空行, 不在节中的行
        }
        else if ( line[0] == '[' )
        {
            // 新的一个节

            char * endsec = std::strchr( line, ']' );

            if ( endsec && endsec - line <= nline )
            {
                // 合法的一个节名

                char * section = line + 1;              // 去掉[
                int32_t nsection = endsec - line - 1;   //

                // 剔除空格, TAB, \r
                section = ConfigSection::trim( section, nsection );

                dosection = new ConfigSection( section, nsection );
                assert( dosection != NULL && "new ConfigSection() failed" );

                // 添加一个节
                bool ret = m_Sections.insert(
                    std::make_pair(dosection->getSection(), dosection) ).second;
                if ( !ret )
                {
                    // 添加失败时, 这个节中的数据全部作废
                    delete dosection;
                    dosection = NULL;
                }
            }
        }
        else
        {
            // 一对合法的项目

            char * endequal = std::strchr( line, '=' );

            if ( endequal && endequal - line <= nline )
            {
                char * key = line;
                char * value = endequal + 1;            // 去掉 =

                int32_t nkey = endequal - line;         // 原始长度
                int32_t nvalue = endline - endequal - 1;// 去掉 = 的1个字节

                // 剔除空格, TAB, \r
                key = ConfigSection::trim( key, nkey );
                value = ConfigSection::trim( value, nvalue );

                dosection->addItem( key, nkey, value, nvalue );
            }
        }

        pos = endline + 1;
    }

    return;
}

const char * ConfigFile::getValue( const char * section, const char * key )
{
    SectionMap::iterator result = m_Sections.find( section );
    if ( result != m_Sections.end() )
    {
        ConfigSection * _section = result->second;
        if ( _section )
        {
            return _section->getValue( key );
        }
    }

    return NULL;
}

bool ConfigFile::get( const char * section, const char * key, bool & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (bool)std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, float & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = std::atof( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, int8_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (int8_t)std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, uint8_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (uint8_t)std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, int16_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (int16_t)std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, uint16_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (uint16_t)std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, int32_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, uint32_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (uint32_t)std::atoi( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, int64_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = std::atoll( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, uint64_t & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = (uint64_t)std::atoll( _value );
        return true;
    }
    return false;
}

bool ConfigFile::get( const char * section, const char * key, std::string & value )
{
    const char * _value = getValue( section, key );
    if ( _value )
    {
        value = _value;
        return true;
    }
    return false;
}

}

#if TEST_UNIT

int main()
{
    Utils::ConfigFile config( "/home/zhangl1/.gitconfig" );

    config.open();

    const char * value = NULL;

    value = config.getValue("pager", "diff" );
    if ( value == NULL )
    {
        printf("not found .\n");
    }
    else
    {
        printf("%s\n", value);
    }

    config.close();
    return 0;
}

#endif
