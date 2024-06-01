#include "thread_cond.h"

using namespace deps;

ThreadCond::ThreadCond(){
	pthread_condattr_t attr;
	int ret = pthread_condattr_init(&attr);
	assert(ret == 0);

	ret = pthread_cond_init(&m_cond, &attr);
	assert(ret == 0);

	ret = pthread_condattr_destroy(&attr);
	assert(ret == 0);
}

ThreadCond::~ThreadCond(){
	int ret = pthread_cond_destroy(&m_cond);
	assert(ret == 0);
}

void ThreadCond::signal(){
	int ret = pthread_cond_signal(&m_cond);
	assert(ret == 0);
}

void ThreadCond::broadcast(){
	int ret = pthread_cond_broadcast(&m_cond);
	assert(ret == 0);
}

//获取绝对等待时间
timespec ThreadCond::abstime(int millsecond)const{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint64_t utime = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec/1000 + (uint64_t)millsecond*1000;
    ts.tv_sec   = utime/1000000;
    ts.tv_nsec  = (utime - (uint64_t)ts.tv_sec*1000000)*1000; 
	return ts;
}

//无限制等待
void ThreadCond::wait(const ThreadMutex& mutex)const{
	int ret = pthread_cond_wait(&m_cond, &mutex.m_mutex);
	assert(ret == 0);
}

//按时间等待
bool ThreadCond::timeWait(const ThreadMutex& mutex, int millsecond)const{
	timespec ts = abstime(millsecond);
	int ret = pthread_cond_timedwait(&m_cond, &mutex.m_mutex, &ts);
	if(ret != 0){
		assert(ret == ETIMEDOUT);
	}
	return true;
}