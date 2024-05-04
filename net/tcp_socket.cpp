#include "tcp_socket.h"

TcpSocket::TcpSocket(SocketContainer *pContainer, PacketHandler* handler){
    m_container = pContainer;
    m_handler = handler;
    m_fd = -1;
    m_state = SocketState::close;
	m_type = SocketType::tcp;
    m_createTime = 0;
    m_lastAccessTime = 0;
    m_timeout = 0;
    m_input = new BlockBuffer<def_block_alloc_4k, 1024>;
    m_output = new BlockBuffer<def_block_alloc_4k, 1024>;
}

TcpSocket::~TcpSocket(){
    m_container = nullptr;
    m_handler = nullptr;
    m_fd = -1;
    m_state = SocketState::close;
	m_type = SocketType::tcp;
    m_createTime = 0;
    m_lastAccessTime = 0;
    m_timeout = 0;
    delete m_input;
    m_input = nullptr;
    delete m_output;
    m_output = nullptr;
}

bool TcpSocket::EnableTcpKeepAlive(int aliveTime, int interval, int count){
    int keepalive = 1;
    if(-1 == setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive, sizeof(keepalive))){
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }

#ifdef __APPLE__
    if (-1 == setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int))) {
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
    if (-1 == setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(int))) {
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
    if (-1 == setsockopt(m_fd, IPPROTO_TCP, TCP_KEEPALIVE, &aliveTime, sizeof(int))) {
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
#else
    if(-1 == setsockopt(m_fd, SOL_TCP, TCP_KEEPIDLE, (void*)&aliveTime, sizeof(aliveTime))){
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
    if(-1 == setsockopt(m_fd, SOL_TCP, TCP_KEEPINTVL, (void*)&interval, sizeof(interval))){
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
    
    if(-1 == setsockopt(m_fd, SOL_TCP, TCP_KEEPCNT, (void*)&count, sizeof(count))){
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
#endif
    

    return true;
}

bool TcpSocket::EnableTcpNoDelay(){
    int nodelay = 1; 
    if(-1 == setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (void*)&nodelay, sizeof(nodelay))){
        LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
        return false;
    }
    return true;
}

void TcpSocket::HandleRead(char* max_read_buffer, size_t max_read_size){
    if (m_state == SocketState::listen) {
        Accept();
    }
    else if (m_state == SocketState::accept || m_state == SocketState::connecting|| m_state == SocketState::connected) {
        Read(max_read_buffer, max_read_size);
    }
    else{
        LOG_INFO("tcp fd:%d socket:%p state:%s can't read", m_fd, this, toString(m_state).c_str());
    }
}

void TcpSocket::HandleWrite()
{
    if (m_state == SocketState::connecting){
        LOG_INFO("tcp fd:%d socket:%p from connecting to connected", m_fd, this);
        m_state = SocketState::connected;
		m_timeout = 0;
		//connected以后只需要关注可读事件
		if(!m_container->ModSocket(this, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
			LOG_ERROR("tcp fd:%d socket:%p mod events failed", m_fd, this);
			Close();
			return;
		}
    }
    else if (m_state == SocketState::accept || m_state == SocketState::connected) {
        Write();
    }
    else{
        LOG_INFO("tcp fd:%d socket:%p state:%s can't write", m_fd, this, toString(m_state).c_str());
    }
}

void TcpSocket::HandleError()
{
    LOG_ERROR("tcp fd:%d socket:%p state:%s error", m_fd, this, toString(m_state).c_str());
    Close();
}

void TcpSocket::HandleTimeout()
{
    time_t now = time(NULL);
    if(m_state == SocketState::accept || m_state == SocketState::connected || m_state == SocketState::connecting){
        if(m_timeout > 0 && m_lastAccessTime > 0 && m_lastAccessTime + m_timeout < now){
            LOG_ERROR("tcp fd:%d socket:%p idle_time:%d timeout:%d", 
				m_fd, this, (int)(now - m_lastAccessTime), m_timeout);
            Close(); 
        }
    }
}

bool TcpSocket::Listen(int port, int backlog, SocketContainer *pContainer, PacketHandler* handler) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
        LOG_ERROR("tcp %s", strerror(errno));
		return false;
	}

	int flags = fcntl(fd, F_GETFL, 0);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		LOG_ERROR("tcp fd:%d %s", fd, strerror(errno));
        return false;
	}

    int reuse = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		LOG_ERROR("tcp fd:%d %s", fd, strerror(errno));
        return false;
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		LOG_ERROR("tcp fd:%d %s", fd, strerror(errno));
        return false;
	}

    if(listen(fd, backlog) == -1){
        LOG_ERROR("tcp fd:%d %s",fd, strerror(errno));
        return false;
    }

    TcpSocket *s = new TcpSocket(pContainer, handler);
    s->SetFd(fd);
    s->SetCreateTime(time(NULL));
    s->SetLastAccessTime(s->GetCreateTime());
    s->SetState(SocketState::listen);
	LOG_DEBUG("tcp fd:%d socket:%p new socket", s->GetFd(), s);
	//listenfd只需要关注可读事件
    if(!pContainer->AddSocket(s, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
        LOG_ERROR("tcp fd:%d socket:%p add events failed", fd, s);
        s->Close();
        return false;
    }

    LOG_INFO("tcp fd:%d socket:%p listen port:%d", fd, s, port);
    return true;
}

SocketBase* TcpSocket::Connect(uint32_t ip, int port, SocketContainer *pContainer, PacketHandler* handler){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1){
        LOG_ERROR("tcp %s %s:%u",strerror(errno), Util::UintIP2String(ip).c_str(), port);
        return nullptr; 
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("tcp fd:%d %s %s:%u",fd,strerror(errno), Util::UintIP2String(ip).c_str(), port);
        return nullptr;
    }

    struct sockaddr_in peerAddr;
    bzero(&peerAddr,sizeof(peerAddr));
    peerAddr.sin_family=AF_INET;
    peerAddr.sin_port=htons(port);
    peerAddr.sin_addr.s_addr=ip;
    int ret = connect(fd,(struct sockaddr *)(&peerAddr),sizeof(struct sockaddr));
    LOG_DEBUG("tcp fd:%d start connecting %s:%u",fd, Util::UintIP2String(ip).c_str(), port);
    TcpSocket *s = new TcpSocket(pContainer, handler);
    s->SetFd(fd);
    s->SetCreateTime(time(NULL));
    s->SetLastAccessTime(s->GetCreateTime());
    s->SetPeerAddr(peerAddr);
	LOG_DEBUG("tcp fd:%d socket:%p new socket %s:%u", s->GetFd(), s, Util::UintIP2String(ip).c_str(), port);
	//connected fd只需要关注可读事件
    if(ret == 0 && pContainer->AddSocket(s, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
        s->SetState(SocketState::connected);
        LOG_DEBUG("tcp fd:%d socket:%p connected %s:%u",s->GetFd(), s, Util::UintIP2String(ip).c_str(), port);
        return s;
    }
	//需要额外关注可写事件，可写这表明连接已经connected
    if(ret == -1 && errno == EINPROGRESS && pContainer->AddSocket(s, SOCKET_EVENT_READ|SOCKET_EVENT_WRITE|SOCKET_EVENT_ERROR)){
        s->SetState(SocketState::connecting);
		s->SetTimeout(TCP_CONNECT_TIMEOUT);
        LOG_INFO("tcp fd:%d socket:%p connect %s:%u in async mode", s->GetFd(),s, Util::UintIP2String(ip).c_str(), port);
        return s;
    }

    LOG_ERROR("tcp fd:%d socket:%p %s %s:%u",fd, s, strerror(errno), Util::UintIP2String(ip).c_str(), port);
    s->Close();
    return nullptr;
}


void TcpSocket::Accept() {
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	int addrLen = sizeof(addr);
	int afd = accept(m_fd, (struct sockaddr*)(&addr),(socklen_t*)&addrLen);
	if (-1 == afd) {
        LOG_ERROR("tcp listenfd:%d %s", m_fd, strerror(errno));
		Close();
        return;
	}

	int flags = fcntl(afd, F_GETFL, 0);
	if (fcntl(afd, F_SETFL, flags | O_NONBLOCK) == -1) {
		LOG_ERROR("tcp listenfd:%d fd:%d %s",m_fd, afd, strerror(errno));
        Close();
        return;
	}

	TcpSocket *s = new TcpSocket(m_container, m_handler);
    s->SetFd(afd);
	s->SetPeerAddr(addr);
    s->SetCreateTime(time(NULL));
	s->SetLastAccessTime(s->GetCreateTime());
	s->SetTimeout(TCP_ACCESS_TIMEOUT);
	s->SetState(SocketState::accept);
	LOG_DEBUG("tcp listenfd:%d fd:%d socket:%p new socket", m_fd, afd, s);
	//只需要关注可读事件
    if(!m_container->AddSocket(s, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
        LOG_ERROR("tcp listenfd:%d fd:%d socket:%p add events failed", m_fd, afd, s);
        s->Close();
        return;
    }

    if(!s->EnableTcpNoDelay()){
        LOG_ERROR("tcp listenfd:%d fd:%d socket:%p set tcp no delay failed", m_fd, afd, s);
        s->Close();
        return;
    }

    LOG_INFO("tcp listenfd:%d fd:%d socket:%p accept", m_fd, afd, s);
}

void TcpSocket::Read(char* max_read_buffer, size_t max_read_size) {
    SetLastAccessTime(time(NULL));
    int n = recv(m_fd, max_read_buffer, max_read_size, 0);
    if (n == -1) {
        if(errno != EAGAIN){
			return;
        }
		else{
			LOG_ERROR("tcp fd:%d socket:%p %s",m_fd, this, strerror(errno));
            Close();
            return;
		}
    }
	else if (n == 0) {
        LOG_INFO("tcp fd:%d socket:%p peer close %s:%u", m_fd, this, 
			inet_ntoa(m_peerAddr.sin_addr), ntohs(m_peerAddr.sin_port));
        Close();
        return;
    }
	else{
		m_input->append(max_read_buffer, n);
		
		int pn = HandlePacket(m_input->data(), m_input->size());

		if(pn > 0){
			m_input->erase(0,pn);
			LOG_DEBUG("tcp fd:%d socket:%p unpack size:%d", m_fd, this, pn);
		}
		else if(pn == 0){
			LOG_DEBUG("tcp fd:%d socket:%p unpack size:0", m_fd, this);
		}
		else{
			//解包失败
			LOG_ERROR("tcp fd:%d socket:%p unpack failed",m_fd, this);
			Close();
		}
		return;
	}
}

void TcpSocket::Write() {
	SetLastAccessTime(time(NULL));
    if(m_output->size() < 1){
        return;
    }

    while(true){
        int n = send(m_fd, m_output->data(), m_output->size(), 0);
        if (n == -1) {
            //没发完的需要等下次可以发的时候继续发
            if(errno == EAGAIN && 
                m_container->ModSocket(this, SOCKET_EVENT_READ|SOCKET_EVENT_WRITE|SOCKET_EVENT_ERROR)){
                LOG_DEBUG("tcp fd:%d socket:%p resend", m_fd, this);
                return;
            }
            else{
                LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
                Close();
                return;
            }
        }
        else{
            m_output->erase(0, n);
            if(m_output->size() <= 0){
                //全部都发完了就不需要再关注可写事件
                if(!m_container->ModSocket(this, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
                    LOG_ERROR("tcp fd:%d socket:%p %s", m_fd, this, strerror(errno));
                    Close();
                }
                return;
            }
        }
    }
}

void TcpSocket::Close(){
	m_handler->HandleClose(this);

    //删除容器存储的数据
    m_container->DelSocket(this);

    if(m_fd != -1){
        //关闭描述符 
        close(m_fd);
    }

    m_fd = -1;
    m_state = SocketState::close;
    m_createTime = 0;
    m_lastAccessTime = 0;
}

bool TcpSocket::SendPacket(const char* data, size_t size){
    if(SocketState::accept != m_state && SocketState::connected != m_state){
        LOG_ERROR("tcp fd:%d socket:%p state:%s can't send", m_fd, this, toString(m_state).c_str());
        return false;
    }
	if(nullptr == data || size < 1){
		return true;
	}
	if(m_output->append(data, size)){
        LOG_DEBUG("tcp fd:%d socket:%p send size:%zd success", m_fd, this, size);
        Write();
        return true;
    }
    return false;
}

int TcpSocket::HandlePacket(const char* data, size_t size){
	return m_handler->HandlePacket(data, size, this);
}