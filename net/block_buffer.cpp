#include "block_buffer.h"

BlockBuffer::BlockBuffer(size_t blocksize, size_t maxblockcount):
    m_blocksize(blocksize), m_maxblockcount(maxblockcount),
    m_blockcount(0), m_capacity(0), m_size(0), m_buffer(nullptr)
{
    if(blocksize < 4096){
        m_blocksize = 4096;
    }
    if(maxblockcount < 50){
        m_maxblockcount = 50;
    }
}

BlockBuffer::~BlockBuffer(){
    if(nullptr != m_buffer){
        delete m_buffer;
        m_buffer = nullptr;
        m_size = 0;
        m_capacity = 0;
        m_blockcount = 0;
    }
}

void BlockBuffer::append(const char* data, size_t size){
    size_t free = m_capacity-m_size;
    if(free >= size){
        memcpy(m_buffer+m_size,data,size);
        m_size += size;
        return;
    }

    size_t needSize = size - free;
    size_t blockcount = needSize/m_blocksize;
    if(m_blocksize * blockcount < needSize){
        blockcount++;
    }
    size_t needCapacity = m_capacity + m_blocksize * blockcount;
    char* tmpbuff = new char[needCapacity];
    assert(tmpbuff != nullptr);
    memcpy(tmpbuff, m_buffer, m_size);
    memcpy(tmpbuff+m_size, data, size);
    m_blockcount += blockcount;
    m_capacity += m_blocksize * blockcount;
    m_size += size;
    delete [] m_buffer;
    m_buffer = tmpbuff;
}

void BlockBuffer::erase(size_t size){
    int eraseSize = size > m_size ? m_size : size;

    size_t free = m_capacity - m_size + eraseSize;
    size_t freeblockcount = free/m_blocksize;
    if(m_blockcount < m_maxblockcount || freeblockcount < 10){
        if(eraseSize < m_size){
            memmove(m_buffer, m_buffer+eraseSize, m_size-eraseSize);
        }
        m_size -= eraseSize;
        return;
    }

    size_t needCapacity = m_capacity - m_blocksize * freeblockcount;
    char* tmpbuff = new char[needCapacity];
    assert(tmpbuff != nullptr);
    if(eraseSize < m_size){
        memcpy(tmpbuff, m_buffer+eraseSize, m_size-eraseSize);
    }
    m_blockcount -= freeblockcount;
    m_capacity -= m_blocksize * freeblockcount;
    m_size -= eraseSize;
    delete [] m_buffer;
    m_buffer = tmpbuff;
}


char* BlockBuffer::data(){
    return m_buffer;
}

size_t BlockBuffer::size(){
    return m_size;
}

size_t BlockBuffer::capacity(){
    return m_capacity;
}

size_t BlockBuffer::blockcount(){
    return m_blockcount;
}