#pragma once

#include <ext/hash_map>

#include "io_buf.h"

typedef __gnu_cxx::hash_map<int, io_buf*> pool_t;	//todo: use c++11 unordered_map

enum MEM_CAP {
	m4K		= 4096,
	m16K	= 16384,
	m64K	= 65536,
	m256K	= 262144,
	m1M		= 1048576,
	m4M		= 4194304,
	m8M	= 8388608
};

//the unit is Kb, so the limit is 5GB
#define EXTRA_MEM_LIMIT	(5U * 1024 * 1024)


/* Singleton of buff_pool class */
class buf_pool
{
public:
	//create instance
	static void init()
	{
		_instance = new buf_pool();
	}

	//get instance of buf_pool
	static buf_pool* instance()
	{
		//ensure init only once
		pthread_once(&_once, init);
		return _instance;
	}
	
	//allocate a io_buf
	io_buf* alloc_buf(int N);

	//revert a io_buf
	void revert(io_buf* buffer);

private:
	buf_pool();

	//private copy ctor, c++11 can use delete
	buf_pool(const buf_pool&);
	buf_pool& operator=(const buf_pool&);

	//handler of buffers
	pool_t _pool;

	//total memory size
	uint64_t _total_mem;

	//singleton
	static buf_pool* _instance;
	
	static pthread_once_t _once;

	static pthread_mutex_t _mutex;
};
