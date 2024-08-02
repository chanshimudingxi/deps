#include "udp_socket.h"

using namespace deps;

UdpSocket::UdpSocket(SocketContainer *pContainer, PacketHandler* handler){
    m_container = pContainer;
    m_handler = handler;
    m_fd = -1;
    m_state = SocketState::close;
	m_type = SocketType::udp;
    m_createTime = 0;
    m_lastAccessTime = 0;
    m_timeout = 0;
    m_input = new BlockBuffer<def_block_alloc_4k, 1024>;
}

UdpSocket::~UdpSocket(){
    m_container = nullptr;
    m_handler = nullptr;
    m_fd = -1;
    m_state = SocketState::close;
	m_type = SocketType::udp;
    m_createTime = 0;
    m_lastAccessTime = 0;
    m_timeout = 0;
    delete m_input;
    m_input = nullptr;
}

void UdpSocket::SetRecvBufferSize(uint32_t size)
{
	uint32_t setSize = size;
	uint32_t oldSize = 0;
	uint32_t optSize  = sizeof(uint32_t);

	getsockopt(m_fd, SOL_SOCKET, SO_RCVBUF,(void *)&oldSize, &optSize); 
    int ret = 0;
#ifdef __APPLE__
    ret = setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, (void *)&setSize, sizeof(setSize));
#else
    ret = setsockopt(m_fd, SOL_SOCKET, SO_RCVBUFFORCE, (void *)&setSize, sizeof(setSize));
#endif
	if( -1 == ret ){
		LOG_ERROR("udp fd:%d recv buffer old size:%u set size:%u failed", m_fd, oldSize, size);
	}

	getsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, (void *)&setSize, &optSize); 

	LOG_INFO("udp fd:%d recv buffer old size:%u set size:%u new size:%u", m_fd, oldSize, size, setSize);
}

void UdpSocket::SetSendBufferSize(uint32_t size)
{
	uint32_t setSize = size;
	uint32_t oldSize = 0;
	uint32_t optSize  = sizeof(uint32_t);

	getsockopt(m_fd, SOL_SOCKET, SO_SNDBUF,(void *)&oldSize, &optSize);
    int ret = 0;
#ifdef __APPLE__
    ret = setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, (void *)&setSize, sizeof(setSize));
#else
    ret = setsockopt(m_fd, SOL_SOCKET, SO_SNDBUFFORCE, (void *)&setSize, sizeof(setSize));
#endif
	if( -1 == ret ){
		LOG_ERROR("udp fd:%d send buffer old size:%u set size:%u failed", m_fd, oldSize, size);
	}

	getsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, (void *)&setSize, &optSize); 

	LOG_INFO("udp fd:%d send buffer old size:%u set size:%u new size:%u", m_fd, oldSize, size, setSize);
}

void UdpSocket::HandleRead(char* max_read_buffer, size_t max_read_size){
	LOG_DEBUG("udp fd:%d socket:%p state:%s read", m_fd, this, toString(m_state).c_str());
	Read(max_read_buffer, max_read_size);
}

void UdpSocket::HandleWrite(){
    LOG_ERROR("udp fd:%d socket:%p state:%s write", m_fd, this, toString(m_state).c_str());
}

void UdpSocket::HandleError(){
    LOG_ERROR("udp fd:%d socket:%p state:%s error", m_fd, this, toString(m_state).c_str());
    Close();
}

void UdpSocket::HandleTimeout(){
    time_t now = time(NULL);
    if(m_state == SocketState::connected){
        if(m_timeout > 0 && m_lastAccessTime > 0 && m_lastAccessTime + m_timeout < now){
            LOG_ERROR("udp fd:%d socket:%p idle_time:%d timeout:%d", 
				m_fd, this, (int)(now - m_lastAccessTime), m_timeout);
            Close(); 
        }
    }
}

bool UdpSocket::Listen(int port, int backlog, SocketContainer *pContainer, PacketHandler* handler){
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
        LOG_ERROR("udp %s", strerror(errno));
		return false;
	}

	int flags = fcntl(fd, F_GETFL, 0);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		LOG_ERROR("udp fd:%d %s", fd, strerror(errno));
        return false;
	}

    int reuse = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
		LOG_ERROR("udp fd:%d %s", fd, strerror(errno));
        return false;
	}

	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		LOG_ERROR("udp fd:%d %s", fd, strerror(errno));
        return false;
	}

    UdpSocket *s = new UdpSocket(pContainer, handler);
    s->SetFd(fd);
    s->SetCreateTime(time(NULL));
    s->SetLastAccessTime(s->GetCreateTime());
    s->SetState(SocketState::listen);
	LOG_DEBUG("udp fd:%d socket:%p new socket", fd, s);
    if(!pContainer->AddSocket(s, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
        LOG_ERROR("fd:%d add events failed", fd);
        s->Close();
        return false;
    }

	s->SetRecvBufferSize(UDP_RECV_BUFF_SIZE);
	s->SetSendBufferSize(UDP_SEND_BUFF_SIZE);
    LOG_INFO("udp fd:%d socket:%p listen port:%d", fd, s, port);
    return true;
}

SocketBase* UdpSocket::Connect(uint32_t ip, int port, SocketContainer *pContainer, PacketHandler* handler){
    int fd = socket(AF_INET,SOCK_DGRAM,0);
    if(fd == -1){
        LOG_ERROR("udp %s %s:%u",strerror(errno), UintIP2String(ip).c_str(), port);
        return nullptr; 
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("udp fd:%d %s %s:%u",fd,strerror(errno), UintIP2String(ip).c_str(), port);
        return nullptr;
    }

    struct sockaddr_in peerAddr;
    bzero(&peerAddr,sizeof(peerAddr));
    peerAddr.sin_family=AF_INET;
    peerAddr.sin_port=htons(port);
    peerAddr.sin_addr.s_addr=ip;

    UdpSocket *s = new UdpSocket(pContainer, handler);
    s->SetFd(fd);
    s->SetCreateTime(time(NULL));
    s->SetLastAccessTime(s->GetCreateTime());
    s->SetPeerAddr(peerAddr);
	LOG_DEBUG("udp fd:%d socket:%p new socket %s:%u", fd, s, UintIP2String(ip).c_str(), port);
    s->SetState(SocketState::connected);
    if(pContainer->AddSocket(s, SOCKET_EVENT_READ|SOCKET_EVENT_ERROR)){
        return s;
    }

    LOG_ERROR("udp fd:%d socket:%p %s %s:%u",fd, s, strerror(errno), UintIP2String(ip).c_str(), port);
    s->Close();
    return nullptr;
}

void UdpSocket::Close(){
    if(m_handler){
        m_handler->HandleClose(this);
    }
    //连接容器中删除描述符
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

void UdpSocket::Read(char* max_read_buffer, size_t max_read_size){
    SetLastAccessTime(time(NULL));
	sockaddr_in sock;
	socklen_t sock_size = sizeof(sock);
    
	int n = recvfrom(m_fd, max_read_buffer, max_read_size, 0, (sockaddr*)(&sock), &sock_size);

    if (n == -1) {
        if(errno != EAGAIN){
			return;
        }
		else{
			LOG_ERROR("udp fd:%d socket:%p %s",m_fd, this, strerror(errno));
            Close();
            return;
		}
    }
	else if (n == 0) {
        LOG_INFO("udp fd:%d socket:%p peer close %s:%u", m_fd, this, 
			inet_ntoa(sock.sin_addr), ntohs(sock.sin_port));
        Close();
        return;
    }
	else{
		m_input->append(max_read_buffer, n);
		
		int pn = 0;
        if(m_handler){
            pn = m_handler->HandlePacket(m_input->data(), m_input->size(), this);
        }

		if(pn > 0){
			m_input->erase(0,pn);
			LOG_DEBUG("udp fd:%d socket:%p unpack size:%d", m_fd, this, pn);
		}
		else if(pn == 0){
			LOG_DEBUG("udp fd:%d socket:%p unpack size:0", m_fd, this);
		}
		else{
			//解包失败
			LOG_ERROR("udp fd:%d socket:%p unpack failed",m_fd, this);
			Close();
		}
		return;
	}
}

bool UdpSocket::SendPacket(const char* data, size_t size){
	SetLastAccessTime(time(NULL));

    if(SocketState::listen != m_state && SocketState::connected != m_state){
        LOG_ERROR("udp fd:%d socket:%p state:%s can't send", m_fd, this, toString(m_state).c_str());
        return false;
    }
	if(nullptr == data || size < 1){
		return true;
	}

	int n = sendto(m_fd, data, size, 0, (struct sockaddr*)&m_peerAddr, sizeof(struct sockaddr));
    if (n == -1) {
        LOG_ERROR("udp fd:%d socket:%p send error:%s", m_fd, this, strerror(errno));
		return false;
    }
	else if(n == 0){
        LOG_ERROR("udp fd:%d socket:%p send size:0 need resend", m_fd, this);
        return false;
    }
	else{
		return true;
	}
}