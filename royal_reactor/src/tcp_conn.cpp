#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#include "tcp_conn.h"
#include "tcp_server.h"
#include "message.h"

//echo business
void callback_busi(const char* data, uint32_t len, int msgid, void* args, tcp_conn *conn)
{
	conn->send_message(data, len, msgid);
}

//read event callback 
static void conn_rd_callback(event_loop* loop, int fd, void* args)
{
	tcp_conn* conn = (tcp_conn*)args;
	conn->do_read();
}

//write event callback
static void conn_wt_callback(event_loop* loop, int fd, void* args)
{
	tcp_conn* conn = (tcp_conn*)args;
	conn->do_write();
}

//ctor
tcp_conn::tcp_conn(int connfd, event_loop* loop)
{
	_connfd = connfd;
	_loop = loop;

	//set connfd non_blocking
	int flag = fcntl(_connfd, F_GETFL, 0);
	fcntl(_connfd, F_SETFL, O_NONBLOCK|flag);

	//set TCP_NODELAY, prohibit reading and write caching to reduce the delay of small packets
	int op = 1;
	setsockopt(_connfd, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));

	if (tcp_server::conn_start_cb)
	{
		tcp_server::conn_start_cb(this, tcp_server::conn_start_cb_args);
	}
	//add read event into event_loop
	_loop->add_io_event(_connfd, conn_rd_callback, EPOLLIN, this);

	tcp_server::increase_conn(_connfd, this);

}

void tcp_conn::do_read()
{
	//read from socket
	int ret = ibuf.read_data(_connfd);
	if (ret == -1)
	{
		fprintf(stderr, "read data from socket\n");
		clean_conn();
		return;
	}
	else if (ret == 0)
	{
		printf("connection closed by peer\n");
		clean_conn();
		return;
	}

	//parsing msg_head data
	msg_head head;

	// may read multiple complete packages at once
	while (ibuf.length() >= MESSAGE_HEAD_LEN)
	{
		//read msg_head, fixed length MESSAGE_HEAD_LEN
		memcpy(&head, ibuf.data(), MESSAGE_HEAD_LEN);
		if (head.msglen > MESSAGE_LENGTH_LIMIT || head.msglen < 0)
		{
			fprintf(stderr, "data format error, need close, msglen = %d\n", head.msglen);
			clean_conn();
			break;
		}
		if (ibuf.length() < MESSAGE_HEAD_LEN + head.msglen)
		{
			//if a incomplete package,discard
				break;
		}
		
		ibuf.pop(MESSAGE_HEAD_LEN);

	   //	printf("read data: %s\n", ibuf.data());

		tcp_server::router.call(head.msgid, head.msglen, ibuf.data(), this);

		//callback_busi(ibuf.data(), head.msglen, head.msgid, nullptr, this);
		
		ibuf.pop(head.msglen);
	}

	ibuf.adjust();

}

void tcp_conn::do_write()
{
	while (obuf.length())
	{
		int ret = obuf.write2fd(_connfd);
		if (ret == -1)
		{
			fprintf(stderr, "write2fd error, close conn!\n");
			clean_conn();
			return;
		}
		else if (ret == 0)
				break;

	}
	
	if (obuf.length() == 0)
	{
		_loop->del_io_event(_connfd, EPOLLOUT);
	}

	return;
}

int tcp_conn::send_message(const char* data, int msglen, int msgid)
{
	//printf("server send_message: %s:%d, msgid = %d\n", data, msglen, msgid);
	bool active_epollout = false;
	if (obuf.length() == 0)
		active_epollout = true;
	
	msg_head head;
	head.msgid = msgid;
	head.msglen = msglen;

	//write message header
	int ret = obuf.send_data((const char*)&head, MESSAGE_HEAD_LEN);
	if (ret == -1)
	{
		fprintf(stderr, "send head error\n");
		return -1;
	}

	//write message body
	ret = obuf.send_data(data, msglen);
	if (ret == -1)
	{
		//if body fail, pop header!
		obuf.pop(MESSAGE_HEAD_LEN);
		return -1;
	}

	if (active_epollout == true)
		_loop->add_io_event(_connfd, conn_wt_callback, EPOLLOUT, this);


	return 0;
}

void tcp_conn::clean_conn()
{
	if (tcp_server::conn_close_cb)
	{
		tcp_server::conn_close_cb(this, tcp_server::conn_close_cb_args);
	}

	tcp_server::decrease_conn(_connfd);
	_loop->del_io_event(_connfd);

	ibuf.clear();
	obuf.clear();

	int fd = _connfd;
	_connfd = -1;

	close(fd);
}


