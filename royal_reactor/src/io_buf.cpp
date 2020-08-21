#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "io_buf.h"

io_buf::io_buf(int size) : next(nullptr), capacity(size), length(0), head(0)
{
	data = new char[size];	
	assert(data);
}

void io_buf::clear()
{
	length = head = 0;
}

void io_buf::adjust()
{
	if (head != 0)
	{
		if (length != 0)
			memmove(data, data+head, length);	// note the diff with memcpy
		head = 0;
	}
}

void io_buf::copy(const io_buf* other)
{
	//note we assumed capacity > other->length
	memcpy(data, other->data+other->head, other->length);
	head = 0;
	length = other->length;
}

void io_buf::pop(int len)
{
	length -= len;
	head += len;
}
