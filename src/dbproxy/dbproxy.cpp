
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "define.h"
#include "base.h"
#include "version.h"

#include "utils/file.h"
#include "utils/timeutils.h"

#include "config.h"
#include "proxyserver.h"

//
RunStatus               g_RunStatus;
Utils::LogFile *        g_Logger;

//
static void signal_handle( int signo );
static void help( const char * module );
static void parse_cmdline( int argc, char ** argv, std::string & module );
static void initialize( const char * module );
static void finitialize();

void signal_handle( int signo )
{
    switch( signo )
    {
        case SIGINT :
        case SIGTERM :
        {
            g_RunStatus = eRunStatus_Stop;
        }
        break;

        case SIGHUP :
        {
            g_RunStatus = eRunStatus_Reload;
        }
        break;
    }
}

void help( const char * module )
{
    printf("%s [-d] [-h|-v] \n", module);
    printf("\t%s --help\n", module);
    printf("\t%s --version\n", module);
    exit(0);
}

void parse_cmdline( int argc, char ** argv, std::string & module )
{
    bool isdaemon = false;

    // 解释命令
    char * result = strrchr( argv[0], '/' );
    module = result+1;

    // 解释参数
    if ( argc > 2 )
    {
        help( module.c_str() );
    }
    else if ( argc == 2 )
    {
        if ( strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0 )
        {
            help( module.c_str() );
        }
        else if ( strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 )
        {
            help( module.c_str() );
        }
        else if ( strcmp(argv[1], "-d") == 0 )
        {
            isdaemon = true;
        }
    }

    // 后台执行
    if ( isdaemon )
    {
        daemon( 1, 0 );
    }

    return;
}

void initialize( const char * module )
{
    bool rc = false;

    // 初始化信号
    signal( SIGPIPE, SIG_IGN );
    signal( SIGINT, signal_handle );
    signal( SIGHUP, signal_handle );
    signal( SIGTERM, signal_handle );

    // 打开全局日志文件
    g_Logger = new Utils::LogFile( "log", module );
    assert( g_Logger != NULL && "new Utils::LogFile failed" );
    rc = g_Logger->open();
    assert( rc && "Utils::LogFile()::open failed" );

    // 加载系统配置
    CDBProxyConfig::getInstance().load( "config/dbproxy.conf" );

    // 设置日志等级
    g_Logger->setLevel( CDBProxyConfig::getInstance().getLogLevel() );
    g_Logger->setMaxSize( CDBProxyConfig::getInstance().getLogFilesize() );

    return;
}

void finitialize()
{
    CDBProxyConfig::getInstance().unload();
    CDBProxyConfig::delInstance();

    g_Logger->close();
    delete g_Logger;
}

int main(int argc, char ** argv)
{
    std::string module;

    // 解释命令行
    parse_cmdline( argc, argv, module );

    // 初始化
    initialize( module.c_str() );

    // 服务开启
    LOG_INFO( "%s-%s start ...\n", module.c_str(), __APPVERSION__ );
    CProxyServer::getInstance().start();
    g_RunStatus = eRunStatus_Running;

    while ( g_RunStatus != eRunStatus_Stop
            && CProxyServer::getInstance().isRunning() )
    {
        if ( g_RunStatus == eRunStatus_Reload )
        {
            // 重新加载配置文件
            LOG_WARN( "%s not support reload configure module .\n", module.c_str() );
            g_RunStatus = eRunStatus_Running;
        }

        Utils::TimeUtils::sleep( 100 );
    }

    // 服务退出
    g_RunStatus = eRunStatus_Stop;
    CProxyServer::getInstance().stop();
    LOG_INFO( "%s-%s stop .\n\n", module.c_str(), __APPVERSION__ );

    // 销毁
    finitialize();

    return 0;
}
