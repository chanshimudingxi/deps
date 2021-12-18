#include "epoll_container.h"

EpollContainer::EpollContainer(int maxFdCount, int maxFdEventWaitTime){
    m_maxFdCount = maxFdCount;
    m_maxFdEventWaitTime = maxFdEventWaitTime;
    m_socketArray = NULL;
    m_epfd = 0;
	m_socketNum = 0;
    m_events = NULL;
}

EpollContainer::~EpollContainer(){
}

bool EpollContainer::AddSocket(SocketBase* s, uint64_t events)
{
    if(nullptr == s){
        return false;
    }
    int fd = s->GetFd();
    m_socketArray[fd] = s;
    struct epoll_event event;
    uint32_t  epollEvents = 0;
    if(SOCKET_EVENT_READ == (events & SOCKET_EVENT_READ)){
        epollEvents = EPOLLIN;
    }
    if(SOCKET_EVENT_WRITE == (events & SOCKET_EVENT_WRITE)){
        epollEvents == 0 ? epollEvents = EPOLLOUT : epollEvents |= EPOLLOUT;
    }
    if(SOCKET_EVENT_ERROR == (events & SOCKET_EVENT_ERROR)){
        epollEvents == 0 ? epollEvents = EPOLLERR : epollEvents |= EPOLLERR;
    }
    event.events = epollEvents;
    event.data.fd = fd;
    if ( -1 == epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &event)){
        return false;
    }
    LOG_DEBUG("fd:%d socket:%p add events:%x", fd, s, epollEvents);
	m_socketNum++;
    return true;
}

bool EpollContainer::ModSocket(SocketBase* s, uint64_t events)
{
    if(nullptr == s){
        return false;
    }
    int fd = s->GetFd();
    if(nullptr == m_socketArray[fd]){
        return false;
    }
    struct epoll_event event;
    uint32_t  epollEvents = 0;
    if(SOCKET_EVENT_READ == (events & SOCKET_EVENT_READ)){
        epollEvents = EPOLLIN;
    }
    if(SOCKET_EVENT_WRITE == (events & SOCKET_EVENT_WRITE)){
        epollEvents == 0 ? epollEvents = EPOLLOUT : epollEvents |= EPOLLOUT;
    }
    if(SOCKET_EVENT_ERROR == (events & SOCKET_EVENT_ERROR)){
        epollEvents == 0 ? epollEvents = EPOLLERR : epollEvents |= EPOLLERR;
    }
    event.events = epollEvents; 
    event.data.fd = fd;
    if ( -1 == epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &event)){
        return false;
    }
    LOG_DEBUG("fd:%d socket:%p mod events:%x", fd, s, epollEvents);
	
    return true;
}

bool EpollContainer::DelSocket(SocketBase* s)
{
    if(nullptr == s){
        return false;
    }
	m_closeSockets.insert(s);
    int fd = s->GetFd();
    m_socketArray[fd] = nullptr;
    //事件容器中删除描述符 
    if( -1 == epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL)){
        return false;
    }
    LOG_DEBUG("fd:%d socket:%p rm events",fd, s);
	m_socketNum--;
    return true;
}

SocketBase* EpollContainer::GetSocket(int fd){
    if(fd < 0 || fd >= m_maxFdCount){
        return nullptr;
    }
    return m_socketArray[fd];
}

bool EpollContainer::Init() {
    //设置程序能够处理的最大描述符个数
	struct rlimit rlim;
	if (getrlimit(RLIMIT_NOFILE, &rlim) == -1) {
        LOG_ERROR("%s",strerror(errno));
        return false;
    }
	rlim.rlim_max = rlim.rlim_cur = m_maxFdCount;
	if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
		LOG_ERROR("%s",strerror(errno));
        return false;
	}

    //初始化连接容器
    m_socketArray = new SocketBase*[m_maxFdCount];
	assert(nullptr != m_socketArray);

	//系统多路复用描述符初始化
	m_events = new struct epoll_event[m_maxFdCount];
	assert(nullptr != m_events);
	m_epfd = epoll_create(m_maxFdCount);
	if (m_epfd == -1) {
        LOG_ERROR("%s",strerror(errno));
		return false;
	}
    
    return true;
}

void EpollContainer::HandleSockets(){
    CheckCloseSocket();
    CheckTimeoutSocket();
    //事件容器里拿出所有描述符
    int ready = epoll_wait(m_epfd, m_events, m_maxFdCount, m_maxFdEventWaitTime);
    if (ready == -1) {
        LOG_ERROR("%s",strerror(errno));
    }
    //处理每个描述符
    for (int i = 0; i < ready; ++i) {
        int fd = m_events[i].data.fd;
        SocketBase *s = m_socketArray[fd];//连接容器里获取描述符对应的连接
        //出现连接容器和事件容器里描述符不一致,出现逻辑bug
        if (nullptr == s) {
            LOG_ERROR("fd:%d socket:%p not in socket container",fd,s);
            continue;
        }
        
        if (EPOLLERR == (m_events[i].events & EPOLLERR)) {
            LOG_ERROR("fd:%d socket:%p error events:%x",fd, s, m_events[i].events);
            s->HandleError();
            continue;
        }		

        if(EPOLLIN == (m_events[i].events & EPOLLIN)){
            LOG_DEBUG("fd:%d socket:%p read events:%x",fd, s, m_events[i].events);
            s->HandleRead();
        }

        if(EPOLLOUT == (m_events[i].events & EPOLLOUT)){
            LOG_DEBUG("fd:%d socket:%p write events:%x",fd, s, m_events[i].events);
            s->HandleWrite();
        }
    }
}

void EpollContainer::CheckTimeoutSocket(){
    //设置了业务层面的心跳超时检测
    static int checkFd=0; 
    time_t now = time(NULL);
    for(int i=0; i < 1000 && i < m_maxFdCount; ++i){
        //利用了操作系统分配描述符的规律，描述符id在[0~maxFdCount)之间
        checkFd = checkFd < m_maxFdCount-1 ? checkFd+1 : 0;
        SocketBase* s = m_socketArray[checkFd];
        if (nullptr != s) {
            s->HandleTimeout();
        }
    }
}

void EpollContainer::CheckCloseSocket(){
	for(std::set<SocketBase*>::iterator it = m_closeSockets.begin(); 
		it != m_closeSockets.end(); ++it){
		SocketBase* pSock = *it;
		if(pSock != nullptr){
			LOG_DEBUG("delete socket:%p fd:%d", pSock, pSock->GetFd());
			delete pSock;
		}
	}
	m_closeSockets.clear();
}

int  EpollContainer::SocketNum(){
	return m_socketNum;
}