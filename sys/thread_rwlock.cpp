#include "thread_rwlock.h"

using namespace deps;

ThreadRWLock::ThreadRWLock(){
	int ret = pthread_rwlock_init(&m_rwlock, NULL);
	assert(ret == 0);
}

ThreadRWLock::~ThreadRWLock(){
	int ret = pthread_rwlock_destroy(&m_rwlock);
	assert(ret == 0);
}

//获取读锁
void ThreadRWLock::ReadLock() const{
	int ret = pthread_rwlock_rdlock(&m_rwlock);
	assert(ret == 0);
}

//获取写锁
void ThreadRWLock::WriteLock() const{
	int ret = pthread_rwlock_wrlock(&m_rwlock);
	assert(ret == 0);
}

//尝试获取读锁
void ThreadRWLock::TryReadLock() const{
	int ret = pthread_rwlock_tryrdlock(&m_rwlock);
	assert(ret == 0);
}

//尝试获取写锁
void ThreadRWLock::TryWriteLock() const{
	int ret = pthread_rwlock_trywrlock(&m_rwlock);
	assert(ret == 0);
}

//解锁
void ThreadRWLock::Unlock() const{
	int ret = pthread_rwlock_unlock(&m_rwlock);
	assert(ret == 0);
}
