#include "epoll_container.h"

using namespace deps;
/**
 * @brief Construct a new fd Container
 * 
 * @param maxFdCount 最大描述符个数
 * @param maxFdEventWaitTime 最长等待事件发生时间
 */
EpollContainer::EpollContainer(int maxFdCount, int maxFdEventWaitTime){
    m_maxFdCount = maxFdCount;
    m_maxFdEventWaitTime = maxFdEventWaitTime;
	m_socketNum = 0;
    
	//系统多路复用描述符初始化
#ifdef __APPLE__
    //创建kqueue，与epoll类似
    m_events = new struct kevent[m_maxFdCount];
    m_epfd = kqueue();
#else
    m_events = new struct epoll_event[m_maxFdCount];
    m_epfd = epoll_create(m_maxFdCount);
#endif
    assert(m_epfd != -1);
}

EpollContainer::~EpollContainer(){
}

bool EpollContainer::AddSocket(SocketBase* s, uint64_t events)
{
    if(m_socketMap.size() >= m_maxFdCount){
        LOG_ERROR("socket container is full");
        return false;
    }

    if(nullptr == s){
        LOG_ERROR("socket is null");
        return false;
    }
    int fd = s->GetFd();
    if(m_socketMap.find(fd) != m_socketMap.end()){
        LOG_ERROR("fd:%d socket:%p already in socket container", fd, s);
        return false;
    }
    
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
        EV_SET(&event[n++], fd, EVFILT_READ, EV_ADD, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents = EPOLLIN;
    #endif
    }
    if(SOCKET_EVENT_WRITE == (events & SOCKET_EVENT_WRITE)){
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)(intptr_t)fd);
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
        LOG_DEBUG("fd:%d socket:%p add events:%lx failed", fd, s, events);
        return false;
    }
    LOG_DEBUG("fd:%d socket:%p add events:%lx success", fd, s, events);
    m_socketMap[fd] = s;
	m_socketNum++;
    return true;
}

bool EpollContainer::ModSocket(SocketBase* s, uint64_t events)
{
    if(nullptr == s){
        LOG_ERROR("socket is null");
        return false;
    }
    int fd = s->GetFd();
    if(m_socketMap.find(fd) == m_socketMap.end()){
        LOG_ERROR("fd:%d socket:%p not in socket container", fd, s);
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
        EV_SET(&event[n++], fd, EVFILT_READ, EV_ADD, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents = EPOLLIN;
    #endif
    }
    else{
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
    #endif
    }

    if(SOCKET_EVENT_WRITE == (events & SOCKET_EVENT_WRITE)){
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)(intptr_t)fd);
    #else
        epollEvents == 0 ? epollEvents = EPOLLOUT : epollEvents |= EPOLLOUT;
    #endif
    }
    else{
    #ifdef __APPLE__
        EV_SET(&event[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)(intptr_t)fd);
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
        #ifdef __APPLE__
        if(ENOENT != errno){//The event could not be found to be modified or deleted.
            LOG_ERROR("fd:%d socket:%p mod events:%lx failed %s", fd, s, events, strerror(errno));
            return false;
        }
        #else
        LOG_ERROR("fd:%d socket:%p mod events:%lx failed %s", fd, s, events, strerror(errno));
        return false;
        #endif
    }
    LOG_DEBUG("fd:%d socket:%p mod events:%lx success", fd, s, events);
    return true;
}

bool EpollContainer::DelSocket(SocketBase* s)
{
    if(nullptr == s){
        return true;
    }
    int fd = s->GetFd();
    //事件容器中删除描述符 
    #ifdef __APPLE__
    //Calling close() on a file descriptor will remove any kevents that reference the descriptor
    // struct kevent event[2];
    #else
    epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL);
    #endif
    auto itr = m_socketMap.find(fd);
    if(itr != m_socketMap.end()){
        m_socketMap.erase(itr);
        m_socketNum--;
    }
    m_closeSockets.insert(s);

    LOG_DEBUG("fd:%d socket:%p rm events",fd, s);
    return true;
}

SocketBase* EpollContainer::GetSocket(int fd){
    auto itr = m_socketMap.find(fd);
    if(itr == m_socketMap.end()){
        return nullptr;
    }
    return itr->second;
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
        
        SocketBase *s = GetSocket(fd);//连接容器里获取描述符对应的连接
        //出现连接容器和事件容器里描述符不一致,出现逻辑bug
        if (nullptr == s) {
            LOG_ERROR("fd:%d socket:%p not in socket container",fd,s);
            continue;
        }
        
        #ifdef __APPLE__
        if (EVFILT_READ == m_events[i].filter) {
            LOG_DEBUG("fd:%d socket:%p kqueue read events:%d",fd, s, m_events[i].filter);
            s->HandleRead(m_maxReadBuffer, MAX_READ_BUFF_SIZE);
        } else if (EVFILT_WRITE == m_events[i].filter) {
            LOG_DEBUG("fd:%d socket:%p kqueue write events:%d",fd, s, m_events[i].filter);
            s->HandleWrite();
        }
        #else
        if (EPOLLERR == (m_events[i].events & EPOLLERR)) {
            LOG_ERROR("fd:%d socket:%p epoll events:%x",fd, s, m_events[i].events);
            s->HandleError();
            continue;
        }

        if(EPOLLIN == (m_events[i].events & EPOLLIN)){
            LOG_DEBUG("fd:%d socket:%p epoll read events:%x",fd, s, m_events[i].events);
            s->HandleRead(m_maxReadBuffer, MAX_READ_BUFF_SIZE);
        }

        if(EPOLLOUT == (m_events[i].events & EPOLLOUT)){
            LOG_DEBUG("fd:%d socket:%p epoll write events:%x",fd, s, m_events[i].events);
            s->HandleWrite();
        }
        #endif
    }
}

void EpollContainer::CheckTimeoutSocket(){
    //设置了业务层面的心跳超时检测
    static int checkFd = 0;
    auto itr = m_socketMap.upper_bound(checkFd);
    int checkCnt = 0;
    while(itr != m_socketMap.end() && checkCnt < 1000){
        checkFd = itr->first;
        SocketBase* s = itr->second;
        if(nullptr != s){
            s->HandleTimeout();
        }
        ++checkCnt;
        ++itr;
    }
    if(itr == m_socketMap.end()){
        checkFd = 0;
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