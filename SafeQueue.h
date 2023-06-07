#pragma once

#include <memory> // Ϊ��ʹ��std::shared_ptr
#include <mutex>
#include <condition_variable>
#include <queue>

template<typename T>
class threadsafe_queue
{
public:
	threadsafe_queue();
	threadsafe_queue(const threadsafe_queue&);
	threadsafe_queue& operator=(
		const threadsafe_queue&) = delete; // ������򵥵ĸ�ֵ

	void push(T new_value);
	bool try_pop(T& value); // 1
	std::shared_ptr<T> try_pop(); // 2
	void wait_and_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	bool empty() const;
private:
	mutable std::mutex mMutex; //lock|unlock in const member function
	std::queue<T> mData_queue;
	std::condition_variable mData_cond;
};
