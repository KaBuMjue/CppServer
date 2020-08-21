#pragma once

#include <queue>
#include <pthread.h>
#include <sys/eventfd.h>
#include <stdio.h>
#include <unistd.h>

#include "event_loop.h"

template <typename T>
class thread_queue
{
public:
	//ctor
	thread_queue() : _loop(nullptr)
	{
		pthread_mutex_init(&_queue_mutex, nullptr);
		_evfd = eventfd(0, EFD_NONBLOCK); // Linux syscall

		if (_evfd == -1)
		{
			perror("eventfd(0, EFD_NONBLOCK)");
			exit(1);
		}
	}

	//dtor
	~thread_queue()
	{
		pthread_mutex_destroy(&_queue_mutex);
		close(_evfd);
	}

	//add a task into queue
	void send(const T& task)
	{
		unsigned long long idle_num = 1;

		pthread_mutex_lock(&_queue_mutex);

		_queue.push(task);

		int ret = write(_evfd, &idle_num, sizeof(unsigned long long));
		if (ret == -1)
		{
			perror("_evfd write");
		}

		pthread_mutex_unlock(&_queue_mutex);
	}

	//get queue (if any task)
	void recv(std::queue<T>& new_queue)
	{
		unsigned long long idle_num = 1;
		pthread_mutex_lock(&_queue_mutex);

		int ret = read(_evfd, &idle_num, sizeof(unsigned long long));
		if (ret == -1)
		{
			perror("_evfd read");
		}

		std::swap(new_queue, _queue);

		pthread_mutex_unlock(&_queue_mutex);
	}

	void set_loop(event_loop* loop)
	{
		_loop = loop;
	}

	void set_callback(io_callback* cb, void* args=nullptr)
	{
		if (_loop != nullptr)
			_loop->add_io_event(_evfd, cb, EPOLLIN, args);
	}

	event_loop* get_loop()
	{
		return _loop;
	}

private:
	int _evfd;
	event_loop* _loop;
	std::queue<T> _queue;
	pthread_mutex_t _queue_mutex;
};
