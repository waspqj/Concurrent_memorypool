#pragma once

#include "common.hpp"

using namespace CommonModule;


class ThreadCache
{
public:
	ThreadCache()
		:_free_list(FREE_LIST_BUCKETS_COUNT)
	{}
	static ThreadCache* GetInstance();

	void* Allocate(size_t size);
	void Deallocate(void* ptr, size_t size);
	
	void* FetchFromCentralCache(size_t index, size_t size);

	~ThreadCache(){}
private:
	std::vector<FreeList> _free_list;
};

static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;