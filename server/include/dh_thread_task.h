/*
 * dh_thread_task.h
 *
 *  Created on: 2013-3-13
 *  Author: Sai
 */

#ifndef DH_THREAD_TASK_H_
#define DH_THREAD_TASK_H_

typedef void * (*TaskFunc)(void * arg);

class DhThreadTask
{
public:
	DhThreadTask(TaskFunc task_func, void* userdata)
	{
		userdata_ = userdata;
		task_func_ = task_func;
	}
	~DhThreadTask()
	{
	}
	void* userdata_;
	TaskFunc task_func_;
};

#endif /* DH_THREAD_TASK_H_ */
