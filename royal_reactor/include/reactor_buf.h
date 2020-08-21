#pragma once

#include <assert.h>
#include <unistd.h>

#include "io_buf.h"
#include "buf_pool.h"

class reactor_buf
{
public:
	reactor_buf();
	~reactor_buf();

	const int length() const;
	void pop(int len);
	void clear();

protected:
	io_buf* _buf;
};


/* read(input) buffer */
class input_buf : public reactor_buf
{
public:
	//read data from a fd
	int read_data(int fd);

	//get data from buf
	const char* data() const;

	//adjust buf
	void adjust();
};

/* write(output) buffer */
class output_buf : public reactor_buf
{
public:
	//write to buffer
	int send_data(const char* data, int datalen);	//
	
	//write to fd
	int write2fd(int fd);
};



