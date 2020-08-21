#include <assert.h>

#include "buf_pool.h"

buf_pool* buf_pool::_instance = nullptr;

pthread_once_t buf_pool::_once = PTHREAD_ONCE_INIT;

pthread_mutex_t buf_pool::_mutex = PTHREAD_MUTEX_INITIALIZER;


/*ctor, allocate a certain amount of space in advance
 *buf_pool is a hashtable, each key is a different space capacity
 *the corresponding value is a linklist of io_buf
 */
buf_pool::buf_pool() : _total_mem(0)
{
	io_buf *prev;

	//allocate a 4K io_buf
	_pool[m4K] = new io_buf(m4K);
	if (_pool[m4K] == nullptr)
	{
		fprintf(stderr, "new io_buf m4K error\n");
		exit(1);
	}

	prev = _pool[m4K];
	//allocate 5000 4K io_bufs in advance, total 20MB
	for (int i = 1; i < 5000; ++i)
	{
		prev->next = new io_buf(m4K);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m4K error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 4 * 5000;


	//allocate a 16K io_buf
	_pool[m16K] = new io_buf(m16K);
	if (_pool[m16K] == nullptr)
	{
		fprintf(stderr, "new io_buf m16K error\n");
		exit(1);
	}

	prev = _pool[m16K];
	// allocate 1000 16K io_bufs in advance, total 16MB
	for (int i = 1; i < 1000; ++i)
	{
		prev->next = new io_buf(m16K);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m16K error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 16 * 1000;

	
	//allocate a 64K io_buf
	_pool[m64K] = new io_buf(m64K);
	if (_pool[m64K] == nullptr)
	{
		fprintf(stderr, "new io_buf m64K error\n");
		exit(1);
	}

	prev = _pool[m64K];
	// allocate 500 64K io_bufs in advance, total 32MB
	for (int i = 1; i < 500; ++i)
	{
		prev->next = new io_buf(m64K);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m64K error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 64 * 500;


	//allocate a 256K io_buf
	_pool[m256K] = new io_buf(m256K);
	if (_pool[m256K] == nullptr)
	{
		fprintf(stderr, "new io_buf m256K error\n");
		exit(1);
	}

	prev = _pool[m256K];
	// allocate 200 256K io_bufs in advance, total 50MB
	for (int i = 1; i < 200; ++i)
	{
		prev->next = new io_buf(m256K);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m256K error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 256 * 200;


	//allocate a 1M io_buf
	_pool[m1M] = new io_buf(m1M);
	if (_pool[m1M] == nullptr)
	{
		fprintf(stderr, "new io_buf m1M error\n");
		exit(1);
	}

	prev = _pool[m1M];
	// allocate 50 1M io_bufs in advance, total 50MB
	for (int i = 1; i < 50; ++i)
	{
		prev->next = new io_buf(m1M);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m1M error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 1024 * 50;


	//allocate a 4M io_buf
	_pool[m4M] = new io_buf(m4M);
	if (_pool[m4M] == nullptr)
	{
		fprintf(stderr, "new io_buf m4M error\n");
		exit(1);
	}

	prev = _pool[m4M];
	// allocate 20 4M io_bufs in advance, total 80MB
	for (int i = 1; i < 20; ++i)
	{
		prev->next = new io_buf(m4M);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m4M error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 4096 * 20;


	//allocate a 8M io_buf
	_pool[m8M] = new io_buf(m8M);
	if (_pool[m8M] == nullptr)
	{
		fprintf(stderr, "new io_buf m16K error\n");
		exit(1);
	}

	prev = _pool[m8M];
	// allocate 10 8M io_bufs in advance, total 80MB
	for (int i = 1; i < 10; ++i)
	{
		prev->next = new io_buf(m8M);
		if (prev->next == nullptr)
		{
			fprintf(stderr, "new io_buf m8M error\n");
			exit(1);
		}
		prev = prev->next;
	}
	_total_mem += 8192 * 10;
}

/*allocate a io_buf
 *(1) if the upper layer need N bytes of space,find the buf group closest to N and take it out.
 *(2) if this buf group has no nodes to use, can allocate additional
 *(3) total_mem should not exceed the limit : EXTRA_MEM_LIMIT
 *(4) if this buf group has node, take it out directly and remove from pool
 */
io_buf* buf_pool::alloc_buf(int N)
{
	//(1) find the closest N buf group
	int index;
	if (N <= m4K)
		index = m4K;
	else if (N <= m16K)
		index = m16K;
	else if (N <= m64K)
		index = m64K;
	else if (N <= m256K)
		index = m256K;
	else if (N <= m1M)
		index = m1M;
	else if (N <= m4M)
		index = m4M;
	else if (N <= m8M)
		index = m8M;
	else
		return nullptr;

	//(2) if no nodes to use, allocate addtional, need to lock
	pthread_mutex_lock(&_mutex);
	if (_pool[index] == nullptr)
	{
		if (_total_mem + index/1024 >= EXTRA_MEM_LIMIT)
		{
			fprintf(stderr, "already use too many memory!\n");
			exit(1);
		}

		io_buf* new_buf = new io_buf(index);
		if (new_buf == nullptr)
		{
			fprintf(stderr, "new io_buf error\n");
			exit(1);
		}
		_total_mem += index/1024;
		pthread_mutex_unlock(&_mutex);
		return new_buf;
	}

	//(3) remove from pool
	io_buf* target = _pool[index];
	_pool[index] = target->next;
	pthread_mutex_unlock(&_mutex);

	target->next = nullptr;

	return target;
}

//revert a io_buf, add to pool
void buf_pool::revert(io_buf* buffer)
{
	int index = buffer->capacity;
	buffer->length = 0;
	buffer->head = 0;

	pthread_mutex_lock(&_mutex);

	assert(_pool.find(index) != _pool.end());

	buffer->next = _pool[index];
	_pool[index] = buffer;
	pthread_mutex_unlock(&_mutex);
}

	

