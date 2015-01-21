
#ifndef __SRC_BASE_DEFINE_H__
#define __SRC_BASE_DEFINE_H__

#include <vector>
#include <string>

#include <stdio.h>
#include <stdint.h>

//==============================================================================
// 框架类型定义
//==============================================================================

// 运行状态
enum RunStatus
{
	eRunStatus_Stop         = 0,	// 退出
	eRunStatus_Running      = 1,    // 正在运行
	eRunStatus_Reload       = 2,    // 重新加载
    eRunStatus_FlushLog     = 3,    // 刷新日志
};

// 主机端信息
struct HostEndpoint
{
    uint16_t    port;
    std::string host;

    void clear()
    {
        port = 0;
        host.clear();
    }
};
typedef std::vector<HostEndpoint> HostEndpoints;

// 定义键值对
typedef std::pair<uint32_t, uint32_t>   Pair;

// 时间周期
typedef std::pair<time_t, time_t>       Period;

// 结果集
typedef std::vector<std::string>        Result;
typedef std::vector<Result>             Results;

enum SQLCmd
{
    eSQLCmd_None        = 0,        // 非法的SQL命令
    eSQLCmd_Insert      = 1,        // 插入
    eSQLCmd_Query       = 2,        // 查询
    eSQLCmd_Update      = 3,        // 更新
    eSQLCmd_Remove      = 4,        // 删除
};

#endif
