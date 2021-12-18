#ifndef _LOCKER_H_
#define _LOCKER_H_


//锁模板类，构造时加锁，析构时解锁。需配合ThreadMutex使用。
template<typename T>
class Locker{
public:
	Locker(const T& mutex):m_mutex(mutex), m_locked(false){
		m_mutex.lock();
		m_locked = true;
	}
	virtual ~Locker(){
		if(m_locked){
			m_mutex.unlock();
		}
	}
    Locker(const Locker&) = delete;
    Locker& operator=(const Locker&) = delete;
protected:
	const T& m_mutex;
	mutable bool m_locked;
};

//读写锁读锁模板类，构造时候加锁，析够的时候解锁
template<typename T>
class RLocker{
public:
	RLocker(T& lock) : m_rwLock(lock), m_locked(false){
		m_rwLock.ReadLock();
		m_locked = true;
	}

	~RLocker(){
		if (m_locked){
			m_rwLock.Unlock();
		}
	}
	RLocker(const RLocker&) = delete;
	RLocker& operator=(const RLocker&) = delete;
private:
	const T& m_rwLock;
    mutable bool m_locked;
};


template <typename T>
class RWLocker{
public:
	RWLocker(T& lock): m_rwLock(lock),m_locked(false){
		m_rwLock.WriteLock();
		m_locked = true;
	}
	~RWLocker(){
		if(m_locked){
			m_rwLock.Unlock();
		}
	}
	RWLocker(const RWLocker&)=delete;
	RWLocker& operator=(const RWLocker&)=delete;
private:
	const T& m_rwLock;
    mutable bool m_locked;

};
#endif