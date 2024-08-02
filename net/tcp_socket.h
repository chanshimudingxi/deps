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
class TcpSocket: public SocketBase{
public:
	static bool Listen(int port, int backlog, SocketContainer *pContainer, PacketHandler* handler);
    static SocketBase* Connect(uint32_t ip, int port, SocketContainer *pContainer, PacketHandler* handler); 

    TcpSocket(SocketContainer *pContainer, PacketHandler* handler);
    ~TcpSocket();
    TcpSocket()=delete;
    TcpSocket(const TcpSocket&)=delete;
    TcpSocket& operator=(const TcpSocket&)=delete;

	virtual void HandleRead(char* max_read_buffer, size_t max_read_size);
	virtual void HandleWrite();
    virtual void HandleError();
    virtual void HandleTimeout();
    virtual bool SendPacket(const char* data, size_t size);
    virtual void Close();
private:	
    void Accept();
    void Read(char* max_read_buffer, size_t max_read_size);
    void Write();
    bool EnableTcpKeepAlive(int aliveTime, int interval, int count);
    bool EnableTcpNoDelay();
    BlockBuffer<def_block_alloc_4k, 1024>* m_input;              //接收缓冲区
    BlockBuffer<def_block_alloc_4k, 1024>* m_output;             //发送缓冲区
    bool m_isResending;                                          //是否正在重发
};
}