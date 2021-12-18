#ifndef SOCKET_BUFFER_H_
#define SOCKET_BUFFER_H_
#include <cstddef>

class SocketBuffer{
public:
    SocketBuffer(){}
    ~SocketBuffer(){}
    
    virtual void append(const char* data, size_t size) = 0;
    virtual void erase(size_t size) = 0;
    virtual char* data() = 0;
    virtual size_t size() = 0;
    virtual size_t capacity() = 0;
};

#endif