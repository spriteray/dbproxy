
#ifndef __SRC_MESSAGE_H__
#define __SRC_MESSAGE_H__

#include <string>
#include <stdint.h>

#pragma pack(1)

struct DBHead
{
    uint16_t    cmd;            // 命令字
    uint32_t    len;            // 长度
    uint32_t    transid;        // 事务ID
};

#pragma pack()

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}

typedef google::protobuf::Message DBMessage;

#endif
