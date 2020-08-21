#pragma once

/* buffer class to store data */
class io_buf
{
public:
	//ctor
	io_buf(int size);

	//clear data
	void clear();

	//clear had processed data, advanced unprocessed data to head
	void adjust();

	//copy data from other buffer
	void copy(const io_buf* other);

	//process data with length len, move head and modify length
	void pop(int len);

	//link to next buffer(if any)
	io_buf* next;

	//buffer capacity
	int capacity;

	//buffer unprocessed data length
	int length;

	//unprocessed data head
	int head;

	//data address
	char* data;
};
