
#ifndef DH_THREAD_POOL_H_
#define DH_THREAD_POOL_H_

#include <vector>
#include <queue>
#include <pthread.h>

#include "dh_thread_task.h"

class DhThread;

//1.暂时不能动态设置线程个数.
class DhThreadPool
{
public:
	DhThreadPool(int max_thread_number);
	~DhThreadPool();
	void AddAsynTask(TaskFunc task_func, void* userdata);
	void Activate(); //1.激活线程池.启动扫描线程.
	void Destroy(); //1.销毁线程池,必然要结束掉所有的线程,等待线程结束.
	void AddThread();
    void MinusThread();
	void Execute();
	void AddFreeThreadToQueue(DhThread* thread);
	bool is_destroy_; //1.通知销毁线程.

private:
	static void* ScanTask(void* userdata);

	void LockTaskQueue();
	void UnlockTaskQueue();

	void LockFreeThreadQueue();
	void UnlockFreeThreadQueue();

	int init_thread_number_; //1.最大正在执行的线程数.
	int maxNum_thread_number_;
    int cur_thread_number;
	std::vector<DhThread*> threads_; //1.线程池容器
	std::queue<DhThread*> free_thread_que_; //1.空闲线程队列
    pthread_mutex_t free_thread_que_mutex_; //1.线程池队列互斥量

	std::queue<DhThreadTask*> task_que_; //1.任务队列
	pthread_mutex_t task_que_mutex_; //1.线程池队列互斥量
	pthread_t task_que_thread_; //1.队列扫描线程,判断线程池里有没有未完成的任务。
};

#endif /* DH_THREAD_POOL_H_ */
