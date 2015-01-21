
#ifndef __SRC_BASE_H__
#define __SRC_BASE_H__

#include <cassert>
#include <cstdlib>

#include "define.h"

#include "utils/file.h"

// 全局状态
extern RunStatus                    g_RunStatus;

// 全局日志模块
extern Utils::LogFile *				g_Logger;

// 日志宏定义
#define LOGGER( level, ... ) 		g_Logger->print( level, __VA_ARGS__ )
#define LOG_FATAL( ... )			LOGGER( Utils::LogFile::eLogLevel_Fatal, __VA_ARGS__ )
#define LOG_ERROR( ... )			LOGGER( Utils::LogFile::eLogLevel_Error, __VA_ARGS__ )
#define LOG_WARN( ... )				LOGGER( Utils::LogFile::eLogLevel_Warn, __VA_ARGS__ )
#define LOG_INFO( ... )				LOGGER( Utils::LogFile::eLogLevel_Info, __VA_ARGS__ )
#define LOG_TRACE( ... )			LOGGER( Utils::LogFile::eLogLevel_Trace, __VA_ARGS__ )
#define LOG_DEBUG( ... )			LOGGER( Utils::LogFile::eLogLevel_Debug, __VA_ARGS__ )

#endif
