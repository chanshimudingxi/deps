#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_

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
#include "block_buffer.h"
#include "sys/log.h"
#include "sys/util.h"

class TcpSocket: public SocketBase{
public:
	static bool Listen(int port, int backlog, SocketContainer *pContainer, ProtoParser* handler);
    static bool Connect(uint32_t ip, int port, SocketContainer *pContainer, ProtoParser* handler, int* connectedfd); 

    TcpSocket(SocketContainer *pContainer, ProtoParser* handler);
    ~TcpSocket();
    TcpSocket()=delete;
    TcpSocket(const TcpSocket&)=delete;
    TcpSocket& operator=(const TcpSocket&)=delete;

	virtual void HandleRead();
	virtual void HandleWrite();
    virtual void HandleError();
    virtual void HandleTimeout();
    virtual bool SendPacket(const char* data, size_t size);
    virtual int HandlePacket(const char* data, size_t size);
    virtual void Close();
private:	
    void Accept();
    void Read();
    void Write();
    bool EnableTcpKeepAlive(int aliveTime, int interval, int count);
    bool EnableTcpNoDelay();
    SocketBuffer* m_input;              //接收缓冲区
    SocketBuffer* m_output;             //发送缓冲区
	char m_buffer[READ_RECV_BUFF_SIZE];
};
#endif
