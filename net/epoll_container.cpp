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
    
    int ret = 0;
#ifdef __APPLE__
    //添加或者修改fd，类似epoll_ctl，但kqueue的read/write两个事件是分开的
    struct kevent event[2];
    int n = 0;
#else
    uint32_t  epollEvents = 0;
    struct epoll_event event;
#endif
    
    if(SOCKET_EVENT_READ == (events & SOCKET_EVENT_READ)){
    #ifdef __APPLE__
        //添加Read事件，EVFILT_READ表示READ事件，操作为添加或者打开，多次重复操作没有副作用
        EV_SET(&event[n++], fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents = EPOLLIN;
    #endif
    }
    if(SOCKET_EVENT_WRITE == (events & SOCKET_EVENT_WRITE)){
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents == 0 ? epollEvents = EPOLLOUT : epollEvents |= EPOLLOUT;
    #endif
    }
    if(SOCKET_EVENT_ERROR == (events & SOCKET_EVENT_ERROR)){
    #ifdef __APPLE__
    #else
        epollEvents == 0 ? epollEvents = EPOLLERR : epollEvents |= EPOLLERR;
    #endif
    }
#ifdef __APPLE__
    //调用kevent，应用更改
    ret = kevent(m_epfd, event, n, NULL, 0, NULL);
#else
    event.events = epollEvents;
    event.data.fd = fd;
    ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, fd, &event);
#endif
    if ( -1 == ret ){
        return false;
    }
    LOG_DEBUG("fd:%d socket:%p add events:%llx", fd, s, events);
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
    int ret = 0;
    #ifdef __APPLE__
    struct kevent event[2];
    int n = 0;
    #else
    struct epoll_event event;
    uint32_t  epollEvents = 0;
    #endif

    if(SOCKET_EVENT_READ == (events & SOCKET_EVENT_READ)){
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents = EPOLLIN;
    #endif
    }
    if(SOCKET_EVENT_WRITE == (events & SOCKET_EVENT_WRITE)){
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_WRITE, EV_ADD|EV_ENABLE, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents == 0 ? epollEvents = EPOLLOUT : epollEvents |= EPOLLOUT;
    #endif
    }
    if(SOCKET_EVENT_ERROR == (events & SOCKET_EVENT_ERROR)){
    #ifdef __APPLE__
    #else
        epollEvents == 0 ? epollEvents = EPOLLERR : epollEvents |= EPOLLERR;
    #endif
    }

#ifdef __APPLE__
    ret = kevent(m_epfd, event, n, NULL, 0, NULL);
#else
    event.events = epollEvents;
    event.data.fd = fd;
    ret = epoll_ctl(m_epfd, EPOLL_CTL_MOD, fd, &event);
#endif

    if ( -1 == ret ){
        return false;
    }
    LOG_DEBUG("fd:%d socket:%p mod events:%llx", fd, s, events);
	
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
    int ret = 0;
    #ifdef __APPLE__
    // struct kevent event[2];
    // int n = 0;
    // //删除Read事件，EVFILT_READ表示READ事件，操作为添加或者打开，多次重复操作没有副作用
    // EV_SET(&event[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    // //删除Read事件，EVFILT_WRITE表示WRITE事件，操作为添加或者打开，多次重复操作没有副作用
    // EV_SET(&event[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    // //调用kevent，应用更改
    // ret = kevent(m_epfd, event, n, NULL, 0, NULL);
    #else
    ret = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL);
    #endif
    if( -1 == ret){
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
	rlim_t oldlimit = rlim.rlim_cur;
	rlim_t oldlimitmax = rlim.rlim_max;
	rlim.rlim_max = rlim.rlim_cur = m_maxFdCount;
	if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
		LOG_ERROR("%s",strerror(errno));
        return false;
	}

    //初始化连接容器
    m_socketArray = new SocketBase*[m_maxFdCount];
	assert(nullptr != m_socketArray);

	//系统多路复用描述符初始化
	
#ifdef __APPLE__
    //创建kqueue，与epoll类似
    m_events = new struct kevent[m_maxFdCount];
    m_epfd = kqueue();
#else
    m_events = new struct epoll_event[m_maxFdCount];
    m_epfd = epoll_create(m_maxFdCount);
#endif
	if (m_epfd == -1) {
        LOG_ERROR("%s",strerror(errno));
		return false;
	}
    
	LOG_INFO("set open files old limit: %llu:%llu to limit: %llu:%llu success", 
		oldlimit, oldlimitmax, m_maxFdCount, m_maxFdCount);
    return true;
}

void EpollContainer::HandleSockets(){
    CheckCloseSocket();
    CheckTimeoutSocket();
    //事件容器里拿出所有描述符
    int ready = 0; 
    #ifdef __APPLE__
    struct timespec timeout;
    timeout.tv_sec = m_maxFdEventWaitTime / 1000;
    timeout.tv_nsec = (m_maxFdEventWaitTime % 1000) * 1000;
    ready = kevent(m_epfd, NULL, 0, m_events, m_maxFdCount, &timeout);
    #else
    ready = epoll_wait(m_epfd, m_events, m_maxFdCount, m_maxFdEventWaitTime);
    #endif

    if (ready == -1) {
        LOG_ERROR("%s",strerror(errno));
    }
    //处理每个描述符
    for (int i = 0; i < ready; ++i) {
        int fd = -1;

        #ifdef __APPLE__
        fd = (int)(intptr_t)m_events[i].udata;
        #else
        fd = m_events[i].data.fd;
        #endif
        
        if(fd < 0 || fd >= m_maxFdCount){
            LOG_ERROR("fd:%d out of range",fd);
            continue;
        }
        SocketBase *s = m_socketArray[fd];//连接容器里获取描述符对应的连接
        //出现连接容器和事件容器里描述符不一致,出现逻辑bug
        if (nullptr == s) {
            LOG_ERROR("fd:%d socket:%p not in socket container",fd,s);
            continue;
        }
        
        #ifdef __APPLE__
        if (EVFILT_READ == m_events[i].filter) {
            LOG_DEBUG("fd:%d socket:%p read events:%x",fd, s, m_events[i].flags);
            s->HandleRead(m_maxReadBuffer, MAX_READ_BUFF_SIZE);
        } else if (EVFILT_WRITE == m_events[i].filter) {
            LOG_DEBUG("fd:%d socket:%p write events:%x",fd, s, m_events[i].flags);
            s->HandleWrite();
        }
        #else
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
        #endif
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