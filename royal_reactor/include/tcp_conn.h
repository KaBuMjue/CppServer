#pragma once

#include "reactor_buf.h"
#include "event_loop.h"
#include "net_connection.h"

// a tcp connection info
class tcp_conn : public net_connection
{
public:
	//ctor
	tcp_conn(int connfd, event_loop* loop);

	//process read
	void do_read();

	//process write
	void do_write();

	//destory connection
	void clean_conn();

	//send message
	int send_message(const char* data, int msglen, int msgid);

private:
	//current connfd
	int _connfd;

	//the event_loop which the conn belongs
	event_loop* _loop;

	//output buffer
	output_buf obuf;

	//input buffer
	input_buf ibuf;
};


