#pragma once

#include <netinet/in.h>

#include "event_loop.h"
#include "tcp_conn.h"
#include "message.h"
#include "thread_pool.h"

class tcp_server
{
public:
	//ctor
	tcp_server(event_loop* loop, const char* ip, uint16_t port);
	
	//provide accept service
	void do_accept();

	//dtor
	~tcp_server();

	//register message router callback
	void add_msg_router(int msgid, msg_callback* cb, void* user_data=nullptr)
	{
		router.register_msg_router(msgid, cb, user_data);
	}

	thread_pool* get_thread_pool()
	{
		return _thread_pool;
	}

private:
	int _sockfd;	//listen socket
	struct sockaddr_in _connaddr;	//client address
	socklen_t _addrlen;		//client address length
	event_loop* _loop;
	thread_pool* _thread_pool;	//thread pool
public:
	//client connection manager
	static void increase_conn(int connfd, tcp_conn* conn);	//add a new connection
	static void decrease_conn(int connfd);		//reduce a connection
	static void get_conn_num(int* curr_conn);	//get current conns numbers
	static tcp_conn** conns;	//all of connections info

public:
	//message router
	static msg_router router;

public:
	//set create conn hook func
	static void set_conn_start(conn_callback cb, void* args=nullptr)
	{
		conn_start_cb = cb;
		conn_start_cb_args = args;
	}

	//set destory conn hook func
	static void set_conn_close(conn_callback cb, void* args=nullptr)
	{
		conn_close_cb = cb;
		conn_close_cb_args = args;
	}

	static conn_callback conn_start_cb;
	static void* conn_start_cb_args;

	static conn_callback conn_close_cb;
	static void* conn_close_cb_args;

private:
#define MAX_CONNS  10000
	static int _max_conns;
	static int _curr_conns;
	static pthread_mutex_t _conns_mutex;
};


