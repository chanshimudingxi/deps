#pragma once

#include <deque>
#include <cassert>
#include <cstdint>
#include <time.h>
#include <errno.h>

#include "locker.h"
#include "thread_mutex.h"
#include "thread_cond.h"

namespace deps{
/**
 * @brief 线程安全队列
 */
template<typename T, typename D = std::deque<T> >
class ThreadQueue
{
public:
    typedef D queue_type;

    ThreadQueue(size_t maxsize):m_maxsize(maxsize){};
	~ThreadQueue(){}
    size_t size() const;
    void clear();
    bool empty() const;
    bool push_front(const T& t);
    bool push_front(const queue_type &qt);
    bool push_back(const T& t);
    bool push_back(const queue_type &qt);
    
	/**
     * @brief 从头部获取数据, 没有数据则等待.
	 * @param millsecond   阻塞等待时间(ms) 0 表示不阻塞 -1 永久等待
     * @return bool: true, 获取了数据, false, 无数据
     */
    bool pop_front(T& t, size_t millsecond = 0);

    /**
	 * @brief  等到有数据才交换. 
	 * @param millsecond  阻塞等待时间(ms) 0 表示不阻塞 -1 如果为则永久等待
     * @return 有数据返回true, 无数据返回false
     */
    bool swap(queue_type &q, size_t millsecond = 0);
protected:
	size_t 				m_maxsize;//队列最大长度
    queue_type          m_queue;//队列
	ThreadMutex			m_mutex;
	ThreadCond			m_cond;
};

template<typename T, typename D> size_t ThreadQueue<T, D>::size() const{
    Locker<ThreadMutex> lock(m_mutex);
    return m_queue.size();
}

template<typename T, typename D> void ThreadQueue<T, D>::clear(){
    Locker<ThreadMutex> lock(m_mutex);
    m_queue.clear();
}

template<typename T, typename D> bool ThreadQueue<T, D>::empty() const{
    Locker<ThreadMutex> lock(m_mutex);
    return m_queue.empty();
}

template<typename T, typename D> bool ThreadQueue<T, D>::push_front(const T& t){
    Locker<ThreadMutex> lock(m_mutex);

	if(m_queue.size()+1 > m_maxsize){
		m_cond.signal();
		return false;
	}
    m_queue.push_front(t);
	m_cond.signal();
	return true;
}

template<typename T, typename D> bool ThreadQueue<T, D>::push_front(const queue_type &qt){
    Locker<ThreadMutex> lock(m_mutex);

	if(m_queue.size() + qt.size() > m_maxsize){
		m_cond.signal();
		return false;
	}

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while(it != itEnd){
        m_queue.push_front(*it);
        ++it;
    }
	m_cond.signal();
	return true;
}

template<typename T, typename D> bool ThreadQueue<T, D>::push_back(const T& t){
    Locker<ThreadMutex> lock(m_mutex);

	if(m_queue.size()+1 > m_maxsize){
		m_cond.signal();
		return false;
	}

    m_queue.push_back(t);
    m_cond.signal();
	return true;
}

template<typename T, typename D> bool ThreadQueue<T, D>::push_back(const queue_type &qt){
    Locker<ThreadMutex> lock(m_mutex);

	if(m_queue.size() + qt.size() > m_maxsize){
		m_cond.signal();
		return false;
	}

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while(it != itEnd){
        m_queue.push_back(*it);
        ++it;
    }
	m_cond.signal();
	return true;
}

template<typename T, typename D> bool ThreadQueue<T, D>::pop_front(T& t, size_t millsecond){
    Locker<ThreadMutex> lock(m_mutex);

	//条件不满足，等待别人唤醒自己
    while (m_queue.empty()){
        if(millsecond == 0){
            return false;
        }
        if(millsecond == (size_t)(-1)){
            m_cond.wait(m_mutex);
        }
        else{
            if(!m_cond.timeWait(m_mutex, millsecond)){
				//超时了
                return false;
            }
        }
    }

	//注意下面的逻辑能够处理是有条件的，那就是ThreadQueue已经被lock了。
	//唤醒自己后，如果条件仍不满足，那就按条件不满足的方式来处理
    if (m_queue.empty()){
        return false;
    }

	//唤醒自己后，如果条件满足，那就按条件满足的方式来处理
    t = m_queue.front();
    m_queue.pop_front();

    return true;
}

template<typename T, typename D> bool ThreadQueue<T, D>::swap(queue_type &q, size_t millsecond){
    Locker<ThreadMutex> lock(m_mutex);

	//条件不满足，等待别人唤醒自己
    while (m_queue.empty()){
        if(millsecond == 0){
            return false;
        }
        if(millsecond == (size_t)(-1)){
            m_cond.wait(m_mutex);
        }
        else{
            if(!m_cond.timeWait(m_mutex, millsecond)){
				//超时了
                return false;
            }
        }
    }

	//唤醒自己后，如果条件仍不满足，那就按条件不满足的方式来处理
    if (m_queue.empty()){
        return false;
    }

    q.swap(m_queue);

    return true;
}
}