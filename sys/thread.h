#ifndef _THREAD_H_
#define _THREAD_H_

#include <cstdint>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "locker.h"
#include "thread_mutex.h"

class Thread{
public:
	Thread():m_running(false), m_tid(-1){}
	virtual ~Thread(){}
	pthread_t start();
	bool isAlive() const;
	pthread_t id() {return m_tid;}
	void join();
	void detach();
	pthread_t id() const;
	void sleep(long millsecond);
	void yield();
protected:
	static void threadEntry(Thread* pThread);
	virtual void run() = 0;
protected:
	bool m_running;
	pthread_t m_tid;
	ThreadMutex m_lock;//防止start函数被同时调用两次产生的问题
};
#endif