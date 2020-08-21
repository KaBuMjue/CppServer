#pragma once
#include "event_loop.h"

typedef void(*task_func)(event_loop* loop, void* args);

struct task_msg
{
	// task type
	enum TASK_TYPE {NEW_CONN, NEW_TASK};
	TASK_TYPE type;

	// task args
	union
	{
		// for NEW_CONN task
		int connfd;

		// for NEW_TASK
		struct
		{
			task_func task_cb;
			void* args;
		};
	};
};
