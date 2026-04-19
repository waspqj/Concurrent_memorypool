#include "ThreadCache.h"

ThreadCache* ThreadCache::GetInstance()
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache();
	}
	return pTLSThreadCache;
}

void* ThreadCache::Allocate(size_t size)
{
	assert(size < MAX_BYTES);
	size_t align_bytes = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);

	if (!_free_list[index].Empty())
	{
		return _free_list[index].Pop();
	}
	else {
		return FetchFromCentralCache(index, align_bytes);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(size < MAX_BYTES);
	assert(ptr);
	size_t align_bytes = SizeClass::RoundUp(size);
	size_t index = SizeClass::Index(size);
	 
	_free_list[index].Push(ptr);
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{

	return nullptr;
}
