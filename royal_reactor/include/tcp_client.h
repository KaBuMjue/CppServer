#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "io_buf.h"
#include "event_loop.h"
#include "message.h"
#include "net_connection.h"

class tcp_client : public net_connection
{
public:
	//ctor
	tcp_client(event_loop* loop, const char* ip, unsigned short port, const char* name);

	//send message
	int send_message(const char* data, int msglen, int msgid);

	//connect
	void do_connect();

	//read
	int do_read();

	//write;
	int do_write();

	//release connection
	void clean_conn();

	//dtor
	~tcp_client();

	void add_msg_router(int msgid, msg_callback* cb, void* user_data=nullptr)
	{
		_router.register_msg_router(msgid, cb, user_data);
	}

	void set_conn_start(conn_callback cb, void* args=nullptr)
	{
		_conn_start_cb = cb;
		_conn_start_cb_args = args;
	}

	void set_conn_close(conn_callback cb, void* args=nullptr)
	{
		_conn_close_cb = cb;
		_conn_close_cb_args = args;
	}

	conn_callback _conn_start_cb;
	void* _conn_start_cb_args;

	conn_callback _conn_close_cb;
	void* _conn_close_cb_args;


	bool connected;		//connect successed?

	struct sockaddr_in _server_addr;	//server addr
	io_buf _ibuf;
	io_buf _obuf;

private:
	int _sockfd;
	socklen_t _addrlen;

	event_loop* _loop;

	const char* _name;	//client name, for log

	msg_router _router;	//message router
};
