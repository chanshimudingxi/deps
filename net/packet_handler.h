#pragma once

#include <cstddef>
#include "socket_base.h"
/*
 * 协议解析器
 */
class SocketBase;

class PacketHandler{
public:
    virtual int	HandlePacket(const char* data, size_t size, SocketBase* s) = 0;
	virtual void HandleClose(SocketBase* s) = 0;
};
