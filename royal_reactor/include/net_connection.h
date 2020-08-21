#pragma once

/* network coommunication abstract class
 * all the recv/send message module need inherit it
 */
class net_connection
{
public:
	virtual int send_message(const char* data, int datalen, int msgid) = 0;
};

typedef void (*conn_callback)(net_connection* conn, void* args);
