#pragma once
#include <memory>
#include <queue>
#include <mutex>
#include <future>

class function_wrapper
{
	struct impl_base {
		virtual void call() = 0;
		virtual ~impl_base() {}
	};
	std::unique_ptr<impl_base> impl;
	template<typename F>
	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)) {}
		void call() { f(); }
	};
public:
	template<typename F>
	function_wrapper(F&& f) :impl(new impl_type<F>(std::move(f)))
	{}

	void operator()() { impl->call(); }
	function_wrapper() = default;
	function_wrapper(function_wrapper&& other) :
		impl(std::move(other.impl))
	{}
	function_wrapper& operator=(function_wrapper&& other)
	{
		impl = std::move(other.impl);
		return *this;
	}
	function_wrapper(const function_wrapper&) = delete;
	function_wrapper(function_wrapper&) = delete;
	function_wrapper& operator=(const function_wrapper&) = delete;
};

template <typename T>
class thread_safe_queue {
public:
	thread_safe_queue() {}

	void enqueue(const T& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(item);
	}
	bool dequeue(T& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (queue_.empty()) {
			return false;
		}
		item = queue_.front();
		queue_.pop();
		return true;
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.empty();
	}

private:
	std::queue<T> queue_;
	mutable std::mutex mutex_;
};

class thread_pool
{
	std::atomic_bool done;
	thread_safe_queue<function_wrapper> work_queue; // ʹ��function_wrapper������ʹ��std:

	void worker_thread()
	{
		while (!done)
		{
			function_wrapper task;
			if (work_queue.dequeue(task))
			{
				task();
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}
public:
	template<typename FunctionType>
	std::future<typename std::result_of<FunctionType()>::type> // 1
		submit(FunctionType f)
	{
		typedef typename std::result_of<FunctionType()>::type
			result_type; // 2
		std::packaged_task<result_type()> task(std::move(f)); // 3
		std::future<result_type> res(task.get_future()); // 4
		work_queue.push(std::move(task)); // 5
		return res; // 6
	}
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return init;
	unsigned long const block_size = 25;
	unsigned long const num_blocks = (length + block_size - 1) / block_size; // 1
	std::vector<std::future<T> > futures(num_blocks - 1);
	thread_pool pool;
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_blocks - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		futures[i] = pool.submit(accumulate_block<Iterator, T>()); // 2
		block_start = block_end;
	}
	T last_result = accumulate_block<Iterator, T>()(block_start, last);
	T result = init;
	for (unsigned long i = 0; i < (num_blocks - 1); ++i)
	{
		result += futures[i].get();
	}
	result += last_result;
	return result;
}

