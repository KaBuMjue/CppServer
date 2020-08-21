#pragma once

#include <ext/hash_map>

#include "net_connection.h"

/* the message header to solve tcp sticky packet problem */
struct msg_head
{
	int msgid;
	int msglen;
};

// the bytes of the meessage header
#define MESSAGE_HEAD_LEN 8

// the length limit of the message header + message body
#define MESSAGE_LENGTH_LIMIT (65535 - MESSAGE_HEAD_LEN)

class tcp_client;
typedef void msg_callback(const char* data, uint32_t len, int msgid, net_connection* net_conn, void* user_data);

// message routing and distribution
class msg_router
{
public:
	msg_router() : _router(), _args()
	{
		printf("msg router init ...\n");
	}
	
	// register a msgid
	int register_msg_router(int msgid, msg_callback* msg_cb, void* user_data)
	{
		//if exist
		if (_router.find(msgid) != _router.end())
			return -1;
		
		printf("add msg cb msgid = %d\n", msgid);

		_router[msgid] = msg_cb;
		_args[msgid] = user_data;

		return 0;
	}

	// call msgid responding callback function
	void call(int msgid, uint32_t msglen, const char* data, net_connection* net_conn)
	{
		if (_router.find(msgid) == _router.end())
		{
			fprintf(stderr, "msgid %d is not register!\n", msgid);
			return;
		}

		msg_callback* callback = _router[msgid];
		void* user_data = _args[msgid];

		callback(data, msglen, msgid, net_conn, user_data);
	}

private:
	// hash_map: msgid -> callback
	__gnu_cxx::hash_map<int, msg_callback*> _router;

	// hash_map: msgid -> args
	__gnu_cxx::hash_map<int, void*> _args;
};
