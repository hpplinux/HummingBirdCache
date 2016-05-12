
#include "../include/dh_thread.h"
#include "../include/dh_thread_pool.h"

#include <iostream>



using namespace std;

DhThread::DhThread(DhThreadPool* pool)
{
	task_ = NULL;
	is_destroy_ = false;
	pool_ = pool;
	thread_id_ = 0;

	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&cond_, NULL);
}

void DhThread::Start()
{
	pthread_create( &thread_, NULL, &DhThread::DoTask, this);
	thread_id_ = (int)thread_;
}

DhThread::~DhThread()
{
	pthread_mutex_destroy(&mutex_);
	pthread_cond_destroy(&cond_);
}

void DhThread::Lock()
{
	pthread_mutex_lock(&mutex_);
}

void DhThread::Unlock()
{
	pthread_mutex_unlock(&mutex_);
}

void DhThread::AddToFreeThreadQueue()
{
	pool_->AddFreeThreadToQueue(this);
}

void DhThread::Wait()
{
	pthread_cond_wait(&cond_, &mutex_);
}

void DhThread::Join()
{
	int res = pthread_join(thread_, NULL);
//	cout << "res:  " << res << endl;

}

void DhThread::Destroy()
{
	Lock();
	is_destroy_ = true;
	Notify();
	Unlock();
	Join();
}

int DhThread::GetId()
{
	return thread_id_;
}

void* DhThread::DoTask(void* userdata)
{
	DhThread* thread = (DhThread*) userdata;
	while (true)
	{
		//thread->Lock();
		if (thread->is_destroy_)
		{
			//thread->Unlock();
			break;
		}
		//thread->Unlock();

		DhThreadTask* task = thread->task_;
		if (task)
		{
			(*task->task_func_)(task->userdata_);
			cout << "task finish: " << thread->GetId() << endl;
			delete task;
			thread->task_ = NULL;
		}


		//thread->Lock();
		if (thread->is_destroy_)
		{
			//thread->Unlock();
			break;
		}

		thread->Lock();
		thread->AddToFreeThreadQueue();
		thread->Wait();
		thread->Unlock();
	}

//	cout << "thread finish: " << thread->GetId() << endl;
	return NULL;
}

void DhThread::Notify()
{
	pthread_cond_signal(&cond_);
}

