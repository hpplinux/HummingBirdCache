
#include "../include/dh_thread_pool.h"
#include <unistd.h>
#include <iostream>
#include "../include/dh_thread.h"
using namespace std;

DhThreadPool::DhThreadPool(int thread_number)
{
	cout << "~DhThreadPool: " << endl;
    init_thread_number_ = thread_number;
	is_destroy_ = false;
    maxNum_thread_number_ = thread_number * 10;
	cur_thread_number = init_thread_number_;
	pthread_mutex_init(&task_que_mutex_, NULL);
	pthread_mutex_init(&free_thread_que_mutex_, NULL);

}
DhThreadPool::~DhThreadPool()
{
//	cout << "~DhThreadPool: " << endl;
	pthread_mutex_destroy(&task_que_mutex_);
	pthread_mutex_destroy(&free_thread_que_mutex_);
}



void DhThreadPool::LockFreeThreadQueue()
{
	pthread_mutex_lock(&free_thread_que_mutex_);
}

void DhThreadPool::UnlockFreeThreadQueue()
{
	pthread_mutex_unlock(&free_thread_que_mutex_);
}


void DhThreadPool::LockTaskQueue()
{
	pthread_mutex_lock(&task_que_mutex_);
}
void DhThreadPool::UnlockTaskQueue()
{
	pthread_mutex_unlock(&task_que_mutex_);
}


void DhThreadPool::AddFreeThreadToQueue(DhThread* thread)
{
	//等待下一个任务.
	LockFreeThreadQueue();
	free_thread_que_.push(thread);
	UnlockFreeThreadQueue();
}

void DhThreadPool::Execute()
{
//	cout << "DhThreadPool::Execute  begin" << endl;
	LockFreeThreadQueue();
	//获取空闲线程数,并且队列里还有未执行的任务.
	//弹出任务.执行任务.
	if (!free_thread_que_.empty())
	{
		LockTaskQueue();
		if (!task_que_.empty())
		{
			DhThreadTask* task = task_que_.front();
			task_que_.pop();

			//1.设置任务.
			DhThread* free_thread = free_thread_que_.front();
			free_thread_que_.pop();
			free_thread->task_ = task;
			//2.通知空闲线程启动.
			free_thread->Notify();
		}
		UnlockTaskQueue();
	}
	UnlockFreeThreadQueue();
//	cout << "DhThreadPool::Execute  end" << endl;
}

void* DhThreadPool::ScanTask(void* userdata)
{
	DhThreadPool* pool = (DhThreadPool*) userdata;
	while (true)
	{
		if (pool->is_destroy_)
		{
			break;
		}
		pool->LockFreeThreadQueue();
		if(pool->free_thread_que_.size() <  (pool->init_thread_number_ / 2)){
			pool->AddThread();
		}else if(pool->free_thread_que_.size() > pool ->init_thread_number_*2){
			pool->MinusThread();
		}
		pool->UnlockFreeThreadQueue();
		pool->Execute();
		sleep(1);
	}

	cout << "DhThreadPool::ScanTask end" << endl;
	return NULL;
}

void DhThreadPool::MinusThread() {

	if(cur_thread_number < init_thread_number_ * 2){
		return;
	}
	for(int i = 0; i < init_thread_number_;i++){

		DhThread* thread = free_thread_que_.front();
		free_thread_que_.pop();
		thread->Destroy();
		delete thread;

	}
	cur_thread_number = cur_thread_number - init_thread_number_;
	cout<< "minus thread, now cur_num: "<< cur_thread_number << endl;

}
void DhThreadPool::AddThread()
{

	int num = 0;
    if(cur_thread_number + init_thread_number_ < maxNum_thread_number_ ){

		num = init_thread_number_ - free_thread_que_.size();
	} else{
		num = maxNum_thread_number_ - cur_thread_number;
	}
	for (int i = 0; i < num; ++i)
	{
		DhThread* thread = new DhThread(this);
//		threads_.push_back(thread);
		thread->Start();
	}
	cur_thread_number = cur_thread_number + num;
	cout<< "add thread, now cur_num: "<< cur_thread_number  << endl;

}
void DhThreadPool::Activate()
{
	cout << "DhThreadPool::activate begin" << endl;
	//暂时先直接创建线程.
	for (int i = 0; i < init_thread_number_; ++i)
	{
		DhThread* thread = new DhThread(this);
//		threads_.push_back(thread);
		thread->Start();
	}
	//启动扫描线程.
	pthread_create(&task_que_thread_, NULL, &ScanTask, this);
}

void DhThreadPool::Destroy()
{
	//1.等待全部线程结束.
	cout << "DhThreadPool::Destroy begin" << endl;

	//1.先停止扫秒线程
	is_destroy_ = true;
	pthread_join(task_que_thread_, NULL);
	cout<< "next1" << endl;

	//	size_t size = threads_.size();

	while(cur_thread_number > 0){
		if(free_thread_que_.empty())
			continue;
		LockFreeThreadQueue();
		DhThread* thread = free_thread_que_.front();
		free_thread_que_.pop();
		thread->Destroy();
		cout << "thread->Destroy()" << endl;
		delete thread;
		cur_thread_number = cur_thread_number - 1;
		cout << "now free_thread size: " << free_thread_que_.size() << "now cur size: "<< cur_thread_number << endl;
		UnlockFreeThreadQueue();

	}
	cout<< "next1" << endl;

	size_t remain = task_que_.size();
	cout << "remain task: " << remain << endl;
	for (size_t i = 0; i < remain; ++i)
	{
		DhThreadTask* task = task_que_.front();
		task_que_.pop();
		delete task;
	}
	cout << "DhThreadPool::Destroy end " << endl;
}

void DhThreadPool::AddAsynTask(TaskFunc task_func, void* userdata)
{
//	cout << "DhThreadPool:: AddAsynTask begin " << endl;
	DhThreadTask *task = new DhThreadTask(task_func, userdata);

	LockTaskQueue();
	//添加任务到队列.
	task_que_.push(task);
	UnlockTaskQueue();
}

