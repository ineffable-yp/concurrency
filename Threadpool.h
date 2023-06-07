#pragma once

#include <stddef.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <set>
#include <functional>
#include <vector>
#include "list.h"

struct threadpool_t
{
	struct list_head task_queue;
	std::mutex mMutex;
	std::condition_variable mCond;
	std::vector<std::thread*> mThreads;
	std::set<std::thread::id> mThreadIDs;
	bool bIsTerminate;
};

struct thrdpool_task
{
	std::function<void(void*)> routine;
	//void (*routine)(void*);
	void* context;
};

struct __thrdpool_task_entry
{
	struct list_head list;
	struct thrdpool_task task;
};

threadpool_t* thrdpool_create(size_t nthreads, size_t stacksize);
int thrdpool_schedule(const struct thrdpool_task* task, threadpool_t* pool);
int thrdpool_increase(threadpool_t* pool);
int thrdpool_in_pool(threadpool_t* pool);
void thrdpool_destroy(std::function<void(const struct thrdpool_task*)> const &pending,threadpool_t* pool);
