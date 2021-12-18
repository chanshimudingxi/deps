#ifndef _THREAD_MUTEX_H_
#define _THREAD_MUTEX_H_

#include <pthread.h>
#include <cassert>
#include <cstdint>
#include <time.h>
#include <errno.h>

/**
 * 线程锁：不可重复加锁，即同一个线程不可以重复加锁 通常不直接使用，和Monitor配合使用
 **/
class ThreadMutex{
public:
	ThreadMutex();
	~ThreadMutex();

    ThreadMutex(const ThreadMutex&)=delete;
    void operator=(const ThreadMutex&)=delete;

	//加锁
	void lock()const;
	//尝试锁
	bool tryLock()const;
	//解锁
	void unlock()const;

friend class ThreadCond;
protected:
    mutable pthread_mutex_t m_mutex;
};
#endif