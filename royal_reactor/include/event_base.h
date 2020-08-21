#pragma once

/* encapsulate IO event */

class event_loop;

/* callback function trigger by IO event */
typedef void io_callback(event_loop* loop, int fd, void* args);

struct io_event
{
	io_event() : read_callback(nullptr), write_callback(nullptr), rcb_args(nullptr),
				 wcb_args(nullptr) {}
	
	int mask;	//EPOLLIN EPOLLOUT
	io_callback* read_callback;		//EPOLLIN event callback
	io_callback* write_callback;	//EPOLLOUT event callback
	void* rcb_args;		//read_callback arguments
	void* wcb_args;		//write_callback arguments

};


