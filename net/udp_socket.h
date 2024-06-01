#pragma once

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <errno.h>
#include <cstring>
#include <arpa/inet.h>

#include "socket_base.h"
#include "blockbuffer.h"
#include "../sys/log.h"
#include "../sys/util.h"

namespace deps{
class UdpSocket : public SocketBase{
public:
	static bool Listen(int port, int backlog, SocketContainer *pContainer, PacketHandler* handler);
	static SocketBase* Connect(uint32_t ip, int port, SocketContainer *pContainer, PacketHandler* handler); 

    UdpSocket(SocketContainer *pContainer, PacketHandler* handler);
    ~UdpSocket();
    UdpSocket()=delete;
    UdpSocket(const UdpSocket&)=delete;
    UdpSocket& operator=(const UdpSocket&)=delete;

	virtual void HandleRead(char* max_read_buffer, size_t max_read_size);
	virtual void HandleWrite();
    virtual void HandleError();
    virtual void HandleTimeout();

    virtual bool SendPacket(const char* data, size_t size);
    virtual int HandlePacket(const char* data, size_t size);
    virtual void Close();
private:
    void Read(char* max_read_buffer, size_t max_read_size);
	void SetRecvBufferSize(uint32_t size);
	void SetSendBufferSize(uint32_t size);
private:
    BlockBuffer<def_block_alloc_4k, 1024>* m_input;              //接收缓冲区
};
}