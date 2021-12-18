#include "thread.h"

void Thread::threadEntry(Thread *pThread){
	pThread->run();
}

//如果start同时被两个线程调用，那么都只会返回真正创建成功的线程
pthread_t Thread::start(){
    Locker<ThreadMutex> sync(m_lock);

    if(m_running){
        return m_tid;
    }

	m_running = true;
    
	int ret = pthread_create(&m_tid, 0, (void *(*)(void *))&threadEntry, (void *)this);

	assert(ret == 0);

    return m_tid;
}

bool Thread::isAlive() const{
    return m_running;
}

void Thread::join(){
	assert(pthread_self() != m_tid);//只能别的线程调

	void* ignore = 0;
	int ret = pthread_join(m_tid, &ignore);

	assert(ret == 0);
}

void Thread::detach(){
	assert(pthread_self() != m_tid);//只能别的线程调

	int ret = pthread_detach(m_tid);
	assert(ret == 0);
}

pthread_t Thread::id() const{
	return m_tid;
}

void Thread::sleep(long millsecond){
	assert(pthread_self() == m_tid);//只能自己调

    struct timespec ts;
    ts.tv_sec = millsecond / 1000;
    ts.tv_nsec = millsecond % 1000 * 1000 * 1000;
	nanosleep(&ts, 0);
}

void Thread::yield(){
	assert(pthread_self() == m_tid);//只能自己调

    sched_yield();
}