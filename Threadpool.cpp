#include "Threadpool.h"

static void __thrdpool_routine(threadpool_t* pool)
{
	struct list_head** pos = &pool->task_queue.next;
	__thrdpool_task_entry* entry;
	std::unique_lock<std::mutex> lk(pool->mMutex, std::defer_lock);//默认不上锁，初始化

	while (1)
	{
		lk.lock();
		while (!pool->bIsTerminate && list_empty(&pool->task_queue))
		{
			pool->mCond.wait(lk);
		}

		if (pool->bIsTerminate)
		{
			break;
		}

		entry = list_entry(pos, __thrdpool_task_entry, list);
		list_del(*pos);
		lk.unlock();

		//execute
		entry->task.routine(entry->task.context);
		delete entry;
	}
}

static int __thrdpool_create(size_t nThreads, size_t nStackSize, threadpool_t* pool)
{
	for (size_t i = 0; i < nThreads; ++i)
	{
		auto* th = new std::thread(__thrdpool_routine, pool);
		pool->mThreads.push_back(th);
		pool->mThreadIDs.emplace(th->get_id());
	}
	return 0;
}

void __thrdpool_schedule(const struct thrdpool_task* task, void* buf, threadpool_t* pool)
{
	__thrdpool_task_entry* entry = (__thrdpool_task_entry*)(buf);
	entry->task = *task;
	std::lock_guard<std::mutex> lk(pool->mMutex);
	list_add_tail(&entry->list, &pool->task_queue);
	pool->mCond.notify_one();
}

threadpool_t* thrdpool_create(size_t nthreads, size_t stacksize)
{
	threadpool_t* pNewPool = new threadpool_t;
	INIT_LIST_HEAD(&pNewPool->task_queue);
	pNewPool->mThreads.clear();
	pNewPool->bIsTerminate = false;
	if (__thrdpool_create(nthreads,stacksize,pNewPool) >= 0)
	{
		return pNewPool;
	}
	delete pNewPool;
	pNewPool = nullptr;
	return pNewPool;
}

int thrdpool_schedule(const thrdpool_task* task, threadpool_t* pool)
{
	__thrdpool_schedule(task, new __thrdpool_task_entry, pool);
	return 0;
}

int thrdpool_increase(threadpool_t* pool)
{
	std::lock_guard <std::mutex> lk(pool->mMutex);
	auto* thr = new std::thread(__thrdpool_routine, pool);
	pool->mThreads.push_back(thr);
	pool->mThreadIDs.emplace(thr->get_id());
	return 0;
}

int thrdpool_in_pool(threadpool_t* pool)
{
	std::lock_guard<std::mutex> lk(pool->mMutex);
	return pool->mThreadIDs.count(std::this_thread::get_id()) > 0;
}

void __thrdpool_terminate(threadpool_t * pool)
{
	std::unique_lock<std::mutex> lk(pool->mMutex);
	for (auto iter : pool->mThreads)
	{
		lk.unlock();
		iter->join();
		lk.lock();
	}
}

void thrdpool_destroy(std::function<void(const struct thrdpool_task*)> const &pending, threadpool_t* pool)
{
	__thrdpool_task_entry* entry;
	struct list_head* pos, * tmp;
	
	__thrdpool_terminate(pool);

	// for (pos = (&pool->task_queue)->next,tmp = pos->next; pos != (&pool->task_queue);
	//	pos = tmp,tmp = pos->next)
	
	list_for_each_safe(pos,tmp,&pool->task_queue)
	{
		entry = list_entry(pos, __thrdpool_task_entry, list);
		list_del(pos);
		if (pending)
			pending(&entry->task);

		delete entry;
	}

	for (auto &th : pool->mThreads)
	{
		delete th;
		th = nullptr;
	}
	delete pool;

}
