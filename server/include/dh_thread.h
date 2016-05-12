
#ifndef DH_THREAD_H_
#define DH_THREAD_H_

#include <pthread.h>
#include "dh_thread_task.h"

class DhThreadPool;

//1.代表一个线程的执行对象.不是代表一个线程.
class DhThread
{
public:
	DhThread(DhThreadPool* pool);
	~DhThread();

	DhThreadTask* task_;
	void AddToFreeThreadQueue();
	void Notify();

	void Lock();
	void Unlock();
	void Wait();
	void Join();
	int GetId();
	void Destroy();

	void Start();

private:
	pthread_mutex_t mutex_;
	pthread_cond_t cond_;

	bool is_destroy_;
	pthread_t thread_;
	DhThreadPool* pool_; //1.所属线程池
	static void* DoTask(void* userdata);

	int thread_id_;
};

#endif /* DH_THREAD_H_ */
