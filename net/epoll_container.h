#ifndef EPOLL_CONTAINER_H_
#define EPOLL_CONTAINER_H_

#include <string>
#include <set>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/resource.h>
#include <errno.h>
#include <cstring>
#include <time.h>
#include <cassert> 

#include "sys/log.h"
#include "socket_base.h"
#include "socket_container.h"

class EpollContainer : public SocketContainer{
public:
	EpollContainer(int maxFdCount, int maxFdEventWaitTime);
	~EpollContainer();
    virtual bool AddSocket(SocketBase* s, uint64_t events);
    virtual bool ModSocket(SocketBase* s, uint64_t events);
    virtual bool DelSocket(SocketBase* s);
    virtual void HandleSockets();
	virtual int  SocketNum();

    SocketBase* GetSocket(int fd);
    bool Init();
private:
    void CheckTimeoutSocket();
	void CheckCloseSocket();
private:
	int m_maxFdCount;//进程能够打开的描述符最大个数
    /* 
    * 用数组管理连接，因为进程能够打开连接小于等于描述符的最大个数
    * 同时依赖内核分配文件描述符的策略是依次递增使用未被使用的描述符，
    * 然后回绕
    */
    SocketBase** m_socketArray;
    int m_epfd;    //管理描述符对应事件的容器
    struct epoll_event *m_events;
    int m_maxFdEventWaitTime; //等待事件发生的最长时间(单位是毫秒)
	std::set<SocketBase*> m_closeSockets;
	int m_socketNum;
};

#endif
