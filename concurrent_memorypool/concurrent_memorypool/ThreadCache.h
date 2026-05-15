#pragma once

#include "common.hpp"
#include "CentralCache.h"

using namespace CommonModule;


class ThreadCache
{
public:
	ThreadCache()
		:_free_list(FREE_LIST_BUCKETS_COUNT)
	{}
	static ThreadCache* GetInstance();

	void* Allocate(size_t size);
	void Deallocate(void* ptr);
	
	void* BigAllocate(size_t size);
	void BigDeallocate(Span* ptr);

	void* FetchFromCentralCache(size_t index, size_t size);
	void FreeListTooLong(FreeList&, size_t);

	~ThreadCache(){}
private:
	std::vector<FreeList> _free_list;

	
};

static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;