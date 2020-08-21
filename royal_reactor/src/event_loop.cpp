#include <assert.h>

#include "event_loop.h"

//ctor
event_loop::event_loop()
{
	//flag = 0 equals epoll_create
	_epfd = epoll_create1(0);

	if (_epfd == -1)
	{
		fprintf(stderr, "epoll_create error\n");
		exit(1);
	}
}

//loop processing event
void event_loop::event_process()
{
	while (true)
	{
		io_event_map_it ev_it;

		int nfds = epoll_wait(_epfd, _fired_evs, MAXEVENTS, 10);
		for (int i = 0; i < nfds; ++i)
		{
			//find the corresponding event through the fd
			ev_it = _io_evs.find(_fired_evs[i].data.fd);
			assert(ev_it != _io_evs.end());

			io_event* ev = &(ev_it->second);

			//read event
			if (_fired_evs[i].events & EPOLLIN)
			{
				void* args = ev->rcb_args;
				ev->read_callback(this, _fired_evs[i].data.fd, args);
			}

			//write event
			else if (_fired_evs[i].events & EPOLLOUT)
			{
				void* args = ev->wcb_args;
				ev->write_callback(this, _fired_evs[i].data.fd, args);
			}

			//note! LT may be trigger this happen? 
			else if (_fired_evs[i].events & (EPOLLHUP|EPOLLERR))
			{
				if (ev->read_callback != nullptr)
				{
					void* args = ev->rcb_args;
					ev->read_callback(this, _fired_evs[i].data.fd, args);
				}
				else if (ev->write_callback != nullptr)
				{
					void* args = ev->wcb_args;
					ev->write_callback(this, _fired_evs[i].data.fd, args);
				}
				else
				{
					fprintf(stderr, "fd %d get error, delete it from epoll\n", _fired_evs[i].data.fd);
					this->del_io_event(_fired_evs[i].data.fd);
				}
			}
		}

		this->execute_ready_tasks();
	}
}

void event_loop::add_io_event(int fd, io_callback* proc, int mask, void* args)
{
	int final_mask;
	int op;
	
	io_event_map_it it = _io_evs.find(fd);
	if (it == _io_evs.end())
	{
		final_mask = mask;
		op = EPOLL_CTL_ADD;
	}
	else
	{
		final_mask = it->second.mask | mask;
		op = EPOLL_CTL_MOD;
	}

	if (mask & EPOLLIN)
	{
		_io_evs[fd].read_callback = proc;
		_io_evs[fd].rcb_args = args;
	}
	else if (mask & EPOLLOUT)
	{
		_io_evs[fd].write_callback = proc;
		_io_evs[fd].wcb_args = args;
	}

	_io_evs[fd].mask = final_mask;

	struct epoll_event event;
	event.events = final_mask;
	event.data.fd = fd;

	if (epoll_ctl(_epfd, op, fd, &event) == -1)
	{
		fprintf(stderr, "epoll ctl %d error\n",fd);
		return;
	}

	listen_fds.insert(fd);
}

void event_loop::del_io_event(int fd)
{
	//

	_io_evs.erase(fd);

	listen_fds.erase(fd);

	epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
}

void event_loop::del_io_event(int fd, int mask)
{
	io_event_map_it it = _io_evs.find(fd);
	if (it == _io_evs.end())
			return;
	int& o_mask = it->second.mask;
	o_mask = o_mask & (~mask);

	if (o_mask == 0)
		this->del_io_event(fd);
	else
	{
		struct epoll_event event;
		event.events = o_mask;
		event.data.fd = fd;
		epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &event);
	}
}

void event_loop::add_task(task_func func, void* args)
{
	task_func_pair func_pair(func, args);
	_ready_tasks.push_back(func_pair);
}

void event_loop::execute_ready_tasks()
{
	std::vector<task_func_pair>::iterator it;

	for (it = _ready_tasks.begin(); it != _ready_tasks.end(); ++it)
	{
		task_func func = it->first;
		void* args = it->second;

		func(this, args);
	}

	_ready_tasks.clear();
}


