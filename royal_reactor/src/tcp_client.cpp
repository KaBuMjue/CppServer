#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <assert.h>
#include <sys/ioctl.h>

#include "tcp_client.h"

tcp_client::tcp_client(event_loop* loop, const char* ip, unsigned short port, const char* name) : _conn_start_cb(nullptr), _conn_start_cb_args(nullptr), _conn_close_cb(nullptr), _conn_close_cb_args(nullptr),	_ibuf(4194304), _obuf(4194304)
{
	_sockfd = -1;
	_name = name;
	_loop = loop;

	bzero(&_server_addr, sizeof(_server_addr));

	_server_addr.sin_family = AF_INET;
	inet_aton(ip, &_server_addr.sin_addr);
	_server_addr.sin_port = htons(port);

	_addrlen = sizeof(_server_addr);

	do_connect();
}

static void write_callback(event_loop* loop, int fd, void* args)
{
	tcp_client* cli = (tcp_client*)args;
	cli->do_write();
}

static void read_callback(event_loop* loop, int fd, void* args)
{
	tcp_client* cli = (tcp_client*)args;
	cli->do_read();
}

static void connection_delay(event_loop* loop, int fd, void* args)
{
	tcp_client* cli = (tcp_client*)args;
	loop->del_io_event(fd);

	int result = 0;
	socklen_t result_len = sizeof(result);
	if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0)
	{
		fprintf(stderr, "get socket option error\n");
		return;
	}
	
	if (result == 0)
	{
		//connect succ!
		cli->connected = true;

		printf("connect %s:%d succ!\n", inet_ntoa(cli->_server_addr.sin_addr),ntohs(cli->_server_addr.sin_port));
		
		if (cli->_conn_start_cb)
		{
			cli->_conn_start_cb(cli, cli->_conn_start_cb_args);
		}


		loop->add_io_event(fd, read_callback, EPOLLIN, cli);
		
		if (cli->_obuf.length != 0)
			loop->add_io_event(fd, write_callback, EPOLLOUT, cli);
	}
	else
		fprintf(stderr, "connection %s:%d error\n", inet_ntoa(cli->_server_addr.sin_addr),ntohs(cli->_server_addr.sin_port));

}


void tcp_client::do_connect()
{
	if (_sockfd != -1)
		close(_sockfd);
	
	//create socket
	_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (_sockfd == -1)
	{
		fprintf(stderr, "create tcp client sockfd error\n");
		exit(1);
	}

	int ret = connect(_sockfd, (struct sockaddr*)&_server_addr, _addrlen);
	if (ret == 0)
	{
		//connect succ!
		connected = true;
		
		if (_conn_start_cb)
		{
			_conn_start_cb(this, _conn_start_cb_args);
		}
		//register read event
		_loop->add_io_event(_sockfd, read_callback, EPOLLIN, this);

		//if write buf is not empty, register write event
		if (_obuf.length != 0)
			_loop->add_io_event(_sockfd, write_callback, EPOLLOUT, this);

		printf("connect %s:%d succ!\n", inet_ntoa(_server_addr.sin_addr),ntohs(_server_addr.sin_port));
	}
	else
	{
		if (errno == EINPROGRESS)
		{
			//if fd is nonblocking, this error don't mean connect failed!
			fprintf(stderr, "do_accept EINPROGRESS\n");

			//register write event, note the callback is different from normal write event!
			_loop->add_io_event(_sockfd, connection_delay, EPOLLOUT, this);
		}
		else
		{
			fprintf(stderr, "connection error\n");
			exit(1);
		}
	}
}



int tcp_client::do_read()
{
	assert(connected == true);

	int need_read = 0;
	if (ioctl(_sockfd, FIONREAD, &need_read) == -1)
	{
		fprintf(stderr, "ioctl FIONREAD error\n");
		return -1;
	}

	assert(need_read <= _ibuf.capacity - _ibuf.length);

	int ret;

	do
	{
		ret = read(_sockfd, _ibuf.data + _ibuf.length, need_read);
	} while(ret == -1 && errno == EINTR);

	if (ret == 0)
	{
		//peer close connection
		if (_name != nullptr)
		{
			printf("%s client: connection close by peer!\n", _name);
		}
		else
		{
			printf("client: connection close by peer!\n");
		}

		clean_conn();
		return -1;
	}

	else if (ret == -1)
	{
		fprintf(stderr, "client: do_read(), error\n");
		clean_conn();
		return -1;
	}

	assert(ret == need_read);
	_ibuf.length += ret;

	msg_head head;
	int msgid, length;

	while (_ibuf.length >= MESSAGE_HEAD_LEN)
	{
		memcpy(&head, _ibuf.data + _ibuf.head, MESSAGE_HEAD_LEN);
		msgid = head.msgid;
		length = head.msglen;

		_ibuf.pop(MESSAGE_HEAD_LEN);

		_router.call(msgid, length, _ibuf.data + _ibuf.head, this);

		_ibuf.pop(length);
	}

	_ibuf.adjust();

	return 0;
}

int tcp_client::do_write()
{
	assert(_obuf.head == 0 && _obuf.length);

	int ret;

	while (_obuf.length)
	{
		do
		{
			ret = write(_sockfd, _obuf.data, _obuf.length);
		} while (ret == -1 && errno == EINTR);

		if (ret > 0)
		{
			_obuf.pop(ret);
			_obuf.adjust();
		}
		else if (ret == -1 && errno != EAGAIN)
		{
			fprintf(stderr, "tcp client write error\n");
			clean_conn();
		}
		else
			break;
	}

	if (_obuf.length == 0)
	{
		//printf("do write over, del EPOLLOUT\n");
		_loop->del_io_event(_sockfd, EPOLLOUT);
	}
	return 0;
}

void tcp_client::clean_conn()
{
	if (_sockfd != -1)
	{
		printf("clean conn, del socket!\n");
		_loop->del_io_event(_sockfd);
		close(_sockfd);
	}

	connected = false;
	
	if (_conn_close_cb)
	{
		_conn_close_cb(this, _conn_close_cb_args);
	}

	//reset connection
	this->do_connect();
}

int tcp_client::send_message(const char* data, int msglen, int msgid)
{
	if (connected == false)
	{
		fprintf(stderr, "no connected, send message stop!\n");
		return -1;
	}

	bool need_add_event = (_obuf.length == 0) ? true : false;
	if (msglen + MESSAGE_HEAD_LEN > _obuf.capacity - _obuf.length)
	{
		fprintf(stderr, "No more space to write socket!\n");
		return -1;
	}

	msg_head head;
	head.msgid = msgid;
	head.msglen = msglen;

	memcpy(_obuf.data + _obuf.length, &head, MESSAGE_HEAD_LEN);
	_obuf.length += MESSAGE_HEAD_LEN;

	memcpy(_obuf.data + _obuf.length, data, msglen);
	_obuf.length +=  msglen;

	if (need_add_event)
		_loop->add_io_event(_sockfd, write_callback, EPOLLOUT, this);

	return 0;
}

tcp_client::~tcp_client()
{
	close(_sockfd);
}
