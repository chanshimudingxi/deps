#pragma once

#include <cstddef>
#include "socket_base.h"
/*
 * 协议解析器
 */
class SocketBase;

class ProtoParser{
public:
    virtual int	HandlePacket(const char* data, size_t size, SocketBase* s) = 0;
};
