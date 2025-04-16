#ifndef MEMORY_POOL_ALLOCATOR_H
#define MEMORY_POOL_ALLOCATOR_H

#include "base.h"
#include "../../sys/sys.h"
#include "../../sys/utils.h"
#include "../number_types.h"
#include "../structures/array.h"
#ifdef _DEBUG
#include "../structures/hash_table.h"
#endif

template <typename T>
struct Memory_Chunk {
	Memory_Chunk *next = NULL;
	T data;
};

template <typename T>
struct Pool_Allocator {
	Pool_Allocator();
	~Pool_Allocator();

	u32 pool_size = 0;

	Memory_Chunk<T> *head = NULL;
	Array<Memory_Chunk<T> *> memory_pools;

#ifdef _DEBUG
	Hash_Table<u64, bool> memory_chunks_table;
#endif

	void init(u32 _pool_size);
	void free_memory();
	T *allocate();

	void free(T *data);
	Memory_Chunk<T> *allocate_new_pool();
};

template<typename T>
Pool_Allocator<T>::Pool_Allocator()
{
}

template<typename T>
Pool_Allocator<T>::~Pool_Allocator()
{
	free_memory();
}

template<typename T>
void Pool_Allocator<T>::init(u32 _pool_size)
{
	assert(_pool_size > 0);

	free_memory();
	pool_size = _pool_size;
}

template<typename T>
void Pool_Allocator<T>::free_memory()
{
	pool_size = 0;
	for (u32 i = 0; i < memory_pools.count; i++) {
		DELETE_ARRAY(memory_pools[i]);
	}
	memory_pools.clear();
#ifdef _DEBUG
	memory_chunks_table.clear();
#endif
}

template<typename T>
T *Pool_Allocator<T>::allocate()
{
	if (!head) {
		head = allocate_new_pool();
	}
	Memory_Chunk<T> *memory_chunk = head;
#ifdef _DEBUG
	u64 address = pointer_address(memory_chunk);
	bool memory_chunk_allocated = false;
	if (memory_chunks_table.get(address, &memory_chunk_allocated)) {
		if (!memory_chunk_allocated) {
			memory_chunks_table[address] = true;
		} else {
			ASSERT_MSG(false, "Pool_Allocator::allocate: Can't allocate memory. An memory chunk has already been allocated.");
		}
	} else {
		ASSERT_MSG(false, "Pool_Allocator::allocate: Can't allocate memory. An address of memory chunk has not been found in an address table.");
	}
#endif
	head = head->next;
	return &memory_chunk->data;
}

template<typename T>
void Pool_Allocator<T>::free(T *data)
{
	assert(data);
	static const u64 POINTER_SIZE = sizeof(u8 *);

	Memory_Chunk<T> *memory_chunk = (Memory_Chunk<T> *)(pointer_address(data) - POINTER_SIZE);
#ifdef _DEBUG
	u64 address = pointer_address(memory_chunk);
	bool memory_chunk_allocated = false;
	if (memory_chunks_table.get(address, &memory_chunk_allocated)) {
		if (memory_chunk_allocated) {
			memory_chunks_table[address] = false;
		} else {
			ASSERT_MSG(false, "Pool_Allocator::free: Can't free memory. An memory chunk has already been freed.");
		}
	} else {
		ASSERT_MSG(false, "Pool_Allocator::free: Can't free memory. The allocator doesn't own this chunk of memory.");
	}
#endif
	//memset((void *)&memory_chunk->data, 0, sizeof(T));
	memory_chunk->data.~T();
	memory_chunk->next = head;
	head = memory_chunk;
}

template<typename T>
Memory_Chunk<T> *Pool_Allocator<T>::allocate_new_pool()
{
	assert(pool_size > 0);

	Memory_Chunk<T> *new_pool = new Memory_Chunk<T>[pool_size];
	for (u32 i = 0; i < pool_size - 1; i++) {
		new_pool[i].next = &new_pool[i + 1];
	}
#ifdef _DEBUG
	for (u32 i = 0; i < pool_size; i++) {
		memory_chunks_table.set(pointer_address(&new_pool[i]), false);
	}
#endif
	memory_pools.push(new_pool);
	return new_pool;
}
#endif
