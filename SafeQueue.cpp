#include "SafeQueue.h"

template<typename T>
void threadsafe_queue<T>::push(T new_value)
{
	std::lock_guard<std::mutex> innerLock(mMutex);
	mData_queue.push(new_value);
	mData_cond.notify_one();
}

template<typename T>
void threadsafe_queue<T>::wait_and_pop(T& value) 
{
	std::unique_lock<std::mutex> innerLock(mMutex);
	mData_cond.wait(innerLock, [] {return !mData_queue.empty(); });
	value = mData_queue.front();
	mData_queue.pop();

	innerLock.unlock();//alternative
}

template<typename T>
bool threadsafe_queue<T>::empty() const 
{
	std::lock_guard<std::mutex> innerLock(mMutex);
	return mData_queue.empty();
}

template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop()
{
	std::unique_lock<std::mutex> innerLock(mMutex);
	mData_cond.wait(innerLock, [] {return !mData_queue.empty(); });
	
	std::shared_ptr<T> resValue = std::make_shared<T>(mData_queue.front());
	mData_queue.pop();
	return resValue;
}

template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop() 
{
	std::lock_guard<std::mutex> innerLock(mMutex);
	if (mData_queue.empty()) return false;
	std::shared_ptr<T> resValue = std::make_shared<T>(mData_queue.front());
	mData_queue.pop();
	return resValue;
}

template<typename T>
bool threadsafe_queue<T>::try_pop(T& value) {
	std::lock_guard<std::mutex> innerLock(mMutex);
	if (mData_queue.empty()) return false;
	value = mData_queue.front();
	mData_queue.pop();
	return true;
}

template<typename T>
threadsafe_queue<T>::threadsafe_queue() 
{

}

template<typename T>
threadsafe_queue<T>::threadsafe_queue(const threadsafe_queue& otherQueue) 
{
	std::lock_guard<std::mutex> innerLock(mMutex);
	mData_queue = otherQueue.mData_queue;
}