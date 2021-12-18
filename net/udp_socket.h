#ifndef UDP_SOCKET_H_
#define UDP_SOCKET_H_

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
#include "sys/log.h"

class UdpSocket : public SocketBase{
public:
	static bool Listen(int port, int backlog, SocketContainer *pContainer, ProtoParser* handler);
	static bool Connect(uint32_t ip, int port, SocketContainer *pContainer, ProtoParser* handler, int* connectedfd); 

    UdpSocket(SocketContainer *pContainer, ProtoParser* handler);
    ~UdpSocket();
    UdpSocket()=delete;
    UdpSocket(const UdpSocket&)=delete;
    UdpSocket& operator=(const UdpSocket&)=delete;

	virtual void HandleRead();
	virtual void HandleWrite();
    virtual void HandleError();
    virtual void HandleTimeout();

    virtual bool SendPacket(const char* data, size_t size);
    virtual int HandlePacket(const char* data, size_t size);
    virtual void Close();
private:	
    void Read();
	void SetRecvBufferSize(uint32_t size);
	void SetSendBufferSize(uint32_t size);
	char m_buffer[READ_RECV_BUFF_SIZE];
};

#endif