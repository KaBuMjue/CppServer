#include <unistd.h>
#include <stdio.h>

#include "thread_pool.h"
#include "event_loop.h"
#include "tcp_conn.h"

void deal_task_message(event_loop* loop, int fd, void* args)
{
	thread_queue<task_msg>* queue = (thread_queue<task_msg>*)args;

	std::queue<task_msg> tasks;
	queue->recv(tasks);

	while (!tasks.empty())
	{
		task_msg task = tasks.front();

		tasks.pop();
		
		// new connection comes
		if (task.type == task_msg::NEW_CONN)
		{
			tcp_conn* conn = new tcp_conn(task.connfd, loop);
			if (conn == nullptr)
			{
				fprintf(stderr, "in thread new tcp_conn error\n");
				exit(1);
			}

			printf("[thread]: get new connection succ!\n");
		}

		else if (task.type == task_msg::NEW_TASK)
		{
			loop->add_task(task.task_cb, task.args);
		}

		else
		{
			fprintf(stderr, "unknow task!\n");
		}
	}
}

// thread main function
void* thread_main(void* args)
{
	thread_queue<task_msg>* queue = (thread_queue<task_msg>*)args;

	// each thread have a event_loop!
	event_loop* loop = new event_loop();
	if (loop == nullptr)
	{
		fprintf(stderr, "new event_loop error\n");
		exit(1);
	}

	queue->set_loop(loop);
	queue->set_callback(deal_task_message, queue);

	loop->event_process();
	
	return nullptr;
}

thread_pool::thread_pool(int thread_cnt) : _queues(nullptr), _thread_cnt(thread_cnt),
		_tids(nullptr), _index(0)
{	
	if (_thread_cnt <= 0)
	{
		fprintf(stderr, "_thread_cnt <= 0\n");
		exit(1);
	}

	_queues = new thread_queue<task_msg>*[thread_cnt];
	_tids = new pthread_t[thread_cnt];

	int ret;

	for (int i = 0; i < _thread_cnt; ++i)
	{
		printf("create %d threads\n",i);

		_queues[i] = new thread_queue<task_msg>();
		ret = pthread_create(&_tids[i], nullptr, thread_main, _queues[i]);
		if (ret == -1)
		{
			perror("thread pool, create thread");
			exit(1);
		}

		pthread_detach(_tids[i]);
	}
}

thread_queue<task_msg>* thread_pool::get_thread()
{
	if (_index == _thread_cnt)
	{
		_index = 0;
	}
	printf("This is %d thread working", _index+1);
	return _queues[_index++];
}

void thread_pool::send_task(task_func func, void* args)
{
	task_msg task;

	for (int i = 0; i < _thread_cnt; ++i)
	{
		task.type = task_msg::NEW_TASK;
		task.task_cb = func;
		task.args = args;

		thread_queue<task_msg>* queue = _queues[i];

		queue->send(task);
	}
}
	
