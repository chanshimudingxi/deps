#pragma once

#include "thread_mutex.h"
#include <cassert>
#include <pthread.h>
#include <cstdint>
#include <time.h>
#include <errno.h>

namespace deps{
/**
 * 线程信号：所有锁可以在上面等待信号发生，和ThreadMutex配合使用。
 * */

class ThreadCond{
public:
	ThreadCond();
	~ThreadCond();
	
	ThreadCond(const ThreadCond&)=delete;
    ThreadCond& operator=(const ThreadCond&)=delete;

	void signal();
	void broadcast();
	//获取绝对等待时间
	timespec abstime(int millsecond)const;
	//无限制等待
	void wait(const ThreadMutex& mutex)const;
	//按时间等待
	bool timeWait(const ThreadMutex& mutex, int millsecond)const;
private:
	mutable pthread_cond_t m_cond;
};
}
