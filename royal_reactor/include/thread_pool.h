#pragma once

#include <pthread.h>

#include "task_msg.h"
#include "thread_queue.h"

class thread_pool
{
public:
	//ctor, thread numbers
	thread_pool(int thread_cnt);

	//get a thread
	thread_queue<task_msg>* get_thread();

	void send_task(task_func func, void* args=nullptr);

private:
	thread_queue<task_msg>** _queues;

	int _thread_cnt;
	
	//thread ids
	pthread_t* _tids;

	int _index;
};
