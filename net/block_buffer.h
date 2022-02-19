#pragma once

#include "socket_buffer.h"
#include <cstddef>
#include <cstring>
#include <cassert> 

class BlockBuffer : public SocketBuffer{
public:
    BlockBuffer(size_t blocksize = 4096, size_t maxblockcount = 50);
    ~BlockBuffer();
    void append(const char* data, size_t size);
    void erase(size_t size);
    char* data();
    size_t size();
    size_t capacity();
    size_t blockcount();
private:
    char* m_buffer;
    size_t m_size;
    size_t m_capacity;
    size_t m_blocksize;
    size_t m_blockcount;
    size_t m_maxblockcount;
};