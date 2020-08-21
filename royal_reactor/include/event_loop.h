#pragma once

/* event loop event processing mechanism */
#include <sys/epoll.h>
#include <ext/hash_map>
#include <ext/hash_set>

#include "event_base.h"
#include "task_msg.h"

#define MAXEVENTS 10

//map: fd -> io_event
typedef __gnu_cxx::hash_map<int, io_event> io_event_map;

//iterator of map
typedef __gnu_cxx::hash_map<int, io_event>::iterator io_event_map_it;

//all the fd set being monitored
typedef __gnu_cxx::hash_set<int> listen_fd_set;

typedef void (*task_func)(event_loop* loop, void* args);

class event_loop
{
public:
	//ctor
	event_loop();

	//loop processing event
	void event_process();

	//add a event into loop
	void add_io_event(int fd, io_callback* proc, int mask, void* args=nullptr);

	//delete a event from loop
	void del_io_event(int fd);

	//delete a event EPOLLIN/EPOLLOUT
	void del_io_event(int fd, int mask);

	void get_listen_fds(listen_fd_set& fds)
	{
		fds = listen_fds;
	}

	void add_task(task_func func, void* args);

	void execute_ready_tasks();

private:
	int _epfd;	//epoll fd

	io_event_map _io_evs;	//map, fd -> io_event

	listen_fd_set listen_fds;	//fd set

	struct epoll_event _fired_evs[MAXEVENTS];	//maximum events can processed at one time

	//ready tasks set
	typedef std::pair<task_func, void*> task_func_pair;
	std::vector<task_func_pair> _ready_tasks;
};







