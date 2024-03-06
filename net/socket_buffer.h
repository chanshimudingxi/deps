#pragma once

#include <cstddef>

class SocketBuffer{
public:
    SocketBuffer(){}
    virtual ~SocketBuffer(){}
    
    virtual void append(const char* data, size_t size) = 0;
    virtual void erase(size_t size) = 0;
    virtual char* data() = 0;
    virtual size_t size() = 0;
    virtual size_t capacity() = 0;
};