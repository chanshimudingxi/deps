#include "thread_mutex.h"

namespace deps{
ThreadMutex::ThreadMutex(){
	int ret;
    pthread_mutexattr_t attr;
    ret = pthread_mutexattr_init(&attr);
    assert(ret == 0);

    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    assert(ret == 0);

    ret = pthread_mutex_init(&m_mutex, &attr);
    assert(ret == 0);

    ret = pthread_mutexattr_destroy(&attr);
    assert(ret == 0);
}

ThreadMutex::~ThreadMutex(){
	int ret = pthread_mutex_destroy(&m_mutex);
	assert(ret == 0);
}

void ThreadMutex::lock()const{
	int ret = pthread_mutex_lock(&m_mutex);
	assert(ret == 0);
}

bool ThreadMutex::tryLock()const{
    int ret = pthread_mutex_trylock(&m_mutex);
    if(ret != 0){
        assert(ret == EBUSY);
    }
    return (ret == 0);
}

void ThreadMutex::unlock()const{
    int ret = pthread_mutex_unlock(&m_mutex);
	assert(ret == 0);
}
}