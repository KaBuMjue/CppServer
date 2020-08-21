#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string>

#include "tcp_client.h"

struct Qps
{
	Qps()
	{
		last_time = time(nullptr);
		succ_cnt = 0;
	}

	long last_time;
	int succ_cnt;
};


void busi(const char* data, uint32_t len, int msgid, net_connection* conn, void* user_data)
{
	Qps* qps = (Qps*)user_data;

	if (strcmp(data, "Hello Royal") == 0)
		qps->succ_cnt ++;
	long current_time = time(nullptr);
	if (current_time - qps->last_time >=1 )
	{
		printf("---> qps = %d <---\n", qps->succ_cnt);
		qps->last_time = current_time;
		qps->succ_cnt = 0;
	}

	conn->send_message(data, len, msgid);
}

void on_client_start(net_connection* conn, void* args)
{
	int msgid = 1;
	const char* msg = "Hello Royal";

	conn->send_message(msg, strlen(msg), msgid);
}

void on_client_lost(net_connection* conn, void* args)
{
	printf("on_client_lost...\n");
	printf("Client is lost!\n");
}

void* thread_main(void* args)
{
	event_loop loop;

	tcp_client client(&loop, "127.0.0.1", 7777, "qps client");

	Qps qps;
	client.add_msg_router(1, busi, (void*)&qps);
   //client.add_msg_router(101, busi);

	client.set_conn_start(on_client_start);
   //client.set_conn_close(on_client_lost);

	loop.event_process();

	return nullptr;
}

int main(int argc, char** argv)
{
	int thread_num = atoi(argv[1]);
	pthread_t* tids;
	tids = new pthread_t[thread_num];

	for (int i = 0; i < thread_num; ++i)
		pthread_create(&tids[i], nullptr, thread_main, nullptr);

	for (int i = 0; i < thread_num; ++i)
		pthread_join(tids[i], nullptr);

	return 0;
}
