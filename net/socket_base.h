#pragma once

#include <netinet/in.h>
#include <time.h>
#include <string>

#include "socket_container.h"
#include "packet_handler.h"

namespace deps{
class SocketContainer;

#define TCP_CONNECT_TIMEOUT  5				//TCP连接超时
#define	TCP_ACCESS_TIMEOUT	60				//TCP不活跃超时
#define UDP_RECV_BUFF_SIZE  16*1024*1024  	//16M
#define UDP_SEND_BUFF_SIZE 	16*1024*1024	//16M
#define MAX_READ_BUFF_SIZE  65536           //一次read最大读取数据，udp包一次没读完数据就被丢了

enum class SocketType{
	tcp,
	udp
};

enum class SocketState {
    connecting,
    connected,
    listen,
	accept,
	close
};


class PacketHandler;

class SocketBase {
public:
    static std::string toString(SocketType type){
        switch(type){
            case SocketType::tcp:
                return "tcp";
            case SocketType::udp:
                return "udp";
            default:
                return "unknow";
        }
    }

    static std::string toString(SocketState state){
        switch(state){
            case SocketState::connecting:
                return "connecting";
            case SocketState::connected:
                return "connected";
            case SocketState::listen:
                return "listen";
            case SocketState::accept:
                return "accept";
            case SocketState::close:
                return "close";
            default:
                return "unknow";
        }
    }
public:
    SocketBase(){}
    virtual ~SocketBase(){}    
	virtual void HandleRead(char* max_read_buffer, size_t max_read_size) = 0;
	virtual void HandleWrite() = 0;
    virtual void HandleError() = 0;
    virtual void HandleTimeout() = 0;
    virtual bool SendPacket(const char* data, size_t size) = 0;
    virtual int HandlePacket(const char* data, size_t size) = 0;
    virtual void Close() = 0;
    void SetFd(int fd){m_fd = fd;}
    int GetFd(){return m_fd;}
    void SetState(SocketState st){m_state = st;}
    SocketState GetState(){return m_state;}
    SocketType GetType(){return m_type;}
    void SetCreateTime(time_t time){m_createTime = time;}
    time_t GetCreateTime(){return m_createTime;}
    void SetLastAccessTime(time_t time){m_lastAccessTime = time;}
    time_t GetLastAccessTime(){return m_lastAccessTime;}
    void SetPeerAddr(struct sockaddr_in addr){m_peerAddr = addr;}
    struct sockaddr_in GetPeerAddr(){return m_peerAddr;}
    void SetTimeout(int timeout){m_timeout = timeout>0 ? timeout :0;}
    int GetTimeout(){return m_timeout;}
protected:
	int m_fd;	                        //描述符id
	SocketState m_state;	            //连接状态
	SocketType  m_type;					//连接类型
	time_t m_createTime;	            //连接创建时间（utc时间）
	time_t m_lastAccessTime;	        //连接上次处理时间（utc时间）
    struct sockaddr_in m_peerAddr;	    //终端端地址
    int m_timeout;                      //连接超时时间（单位是秒，0表示永不超时）
    SocketContainer *m_container;      	//容器
    PacketHandler* m_handler;           //协议解析
};
}