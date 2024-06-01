#pragma once

#include "socket_base.h"

namespace deps{
const uint64_t SOCKET_EVENT_READ = 1;
const uint64_t SOCKET_EVENT_WRITE = 2;
const uint64_t SOCKET_EVENT_ERROR = 4;

class SocketBase;

class SocketContainer
{
public:
    SocketContainer(){}
    virtual ~SocketContainer(){}
    virtual bool AddSocket(SocketBase* s, uint64_t events) = 0;
    virtual bool ModSocket(SocketBase* s, uint64_t events) = 0;
    virtual bool DelSocket(SocketBase* s)  = 0;
	virtual void HandleSockets() = 0;
	virtual int  SocketNum() = 0;
};
}