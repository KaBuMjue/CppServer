#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "tcp_server.h"
#include "reactor_buf.h"
#include "config_file.h"

tcp_conn** tcp_server::conns = nullptr;

msg_router tcp_server::router;

conn_callback tcp_server::conn_start_cb = nullptr;
void* tcp_server::conn_start_cb_args = nullptr;

conn_callback tcp_server::conn_close_cb = nullptr;
void* tcp_server::conn_close_cb_args = nullptr;

int tcp_server::_max_conns = 0;

int tcp_server::_curr_conns = 0;

pthread_mutex_t tcp_server::_conns_mutex = PTHREAD_MUTEX_INITIALIZER;

void tcp_server::increase_conn(int connfd, tcp_conn* conn)
{
	pthread_mutex_lock(&_conns_mutex);
	conns[connfd] = conn;
	++_curr_conns;
	pthread_mutex_unlock(&_conns_mutex);
}

void tcp_server::decrease_conn(int connfd)
{
	pthread_mutex_lock(&_conns_mutex);
	conns[connfd] = nullptr;
	--_curr_conns;
	pthread_mutex_unlock(&_conns_mutex);
}

void tcp_server::get_conn_num(int* curr_conn)
{
	pthread_mutex_lock(&_conns_mutex);
	*curr_conn = _curr_conns;
	pthread_mutex_unlock(&_conns_mutex);
}


void accept_callback(event_loop* loop, int fd, void* args)
{
	tcp_server* server = (tcp_server*)args;
	server->do_accept();
}

//ctor
tcp_server::tcp_server(event_loop* loop, const char* ip, uint16_t port)
{
	bzero(&_connaddr, sizeof(_connaddr));

	//ignore some signals: SIGHUP, SIGPIPE
	if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
		fprintf(stderr, "signal ignore SIGHUP\n");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		fprintf(stderr, "signal ignore SIGPIPE\n");
	
	//create socket
	_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (_sockfd == -1)
	{
		fprintf(stderr, "tcp_server::socket()\n");	//todo: use __FILE__, __LINE__
		exit(1);
	}

	//init server address
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_aton(ip, &server_addr.sin_addr);
	server_addr.sin_port = htons(port);

	//set REUSE attr
	int op = 1;
	if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0)
		fprintf(stderr, "setsockopt SO_REUSEADDR\n");

	//bind
	if (bind(_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		fprintf(stderr, "bind error\n");
		exit(1);
	}

	//listen
	if (listen(_sockfd, 500) == -1)
	{
		fprintf(stderr, "listen error\n");
		exit(1);
	}

	//add _sockfd into event_loop
	_loop = loop;
	
	//create connections info array
	_max_conns = config_file::instance()->GetNumber("reactor", "maxConn",1024);
	conns = new tcp_conn*[_max_conns+3];	//+3 -> stdin, stdout, stderr
	if (conns == nullptr)
	{
		fprintf(stderr, "new conns[%d] error\n", _max_conns);
		exit(1);
	}
	bzero(conns, sizeof(tcp_conn*)*(_max_conns+3));

	int thread_cnt = config_file::instance()->GetNumber("reactor","threadNum",10);;
	if (thread_cnt > 0)
	{
		_thread_pool = new thread_pool(thread_cnt);
		if (_thread_pool == nullptr)
		{
			fprintf(stderr, "tcp_server new thread_pool error\n");
			exit(1);
		}
	}
	_loop->add_io_event(_sockfd, accept_callback, EPOLLIN, this);
	
}


void tcp_server::do_accept()
{
	int connfd;
	
	while (true)
	{
		printf("begin accept\n");
		connfd = accept(_sockfd, (struct sockaddr*)&_connaddr, &_addrlen);
		if (connfd == -1)
		{
			if (errno == EINTR)
			{
				fprintf(stderr, "accept errno = EINTR\n");
				continue;
			}
			else if (errno == EMFILE)
			{
				fprintf(stderr, "accept errno = EMFILE\n");

			}
			else if (errno == EAGAIN)
			{
				fprintf(stderr, "accept errno = EAGAIN\n");
				break;
			}
			else
			{
				fprintf(stderr, "accept error\n");
				exit(1);
			}
		}

		else
		{
			//accept succ!
			int cur_conns;
			get_conn_num(&cur_conns);

			if (cur_conns >= _max_conns)
			{
				fprintf(stderr, "so many connections, max = %d\n", _max_conns);
				close(connfd);
			}
			else
			{
				// mutiple threads
				if (_thread_pool != nullptr)
				{
					thread_queue<task_msg>* queue = _thread_pool->get_thread();

					task_msg task;
					task.type = task_msg::NEW_CONN;
					task.connfd = connfd;

					queue->send(task);
				}
				// single thread
				else
				{
					tcp_conn *conn = new tcp_conn(connfd, _loop);
					if (conn == nullptr)
					{
						fprintf(stderr, "new tcp_conn error\n");
						exit(1);
					}
					printf("[tcp_server]: get new connection succ!\n");
			
					break;
				}
			}
		}
	}
}

tcp_server::~tcp_server()
{
	_loop->del_io_event(_sockfd);

	close(_sockfd);
}






