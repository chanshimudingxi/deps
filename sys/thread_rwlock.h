#pragma once

#include <cassert>
#include <pthread.h>
#include <cstdint>
#include <time.h>
#include <errno.h>

class ThreadRWLock{
public:
	ThreadRWLock();
	~ThreadRWLock();
	ThreadRWLock(const ThreadRWLock&) = delete;
	ThreadRWLock& operator=(const ThreadRWLock&)=delete;
	//获取读锁
	void ReadLock() const;
	//获取写锁
	void WriteLock() const;
	//尝试获取读锁
	void TryReadLock() const;
	//尝试获取写锁
	void TryWriteLock() const;
	//解锁
	void Unlock() const;
private:
	mutable pthread_rwlock_t m_rwlock;
};