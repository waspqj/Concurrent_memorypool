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
	size_t index = SizeClass::Index(size);
	 
	_free_list[index].Push(ptr);
	//如果链表长度太长，归还一些对象给中心缓存
	if (_free_list[index]._MaxSize() < _free_list[index]._Size())
	{
		FreeListTooLong(_free_list[index], index);
	}
}

void ThreadCache::FreeListTooLong(FreeList& list, size_t index)
{
	//把一串对象从freelist里拿出来
	void* start = nullptr;
    void* end = nullptr;
	list.Pop(list._MaxSize(), start, end);
	//批量归还给中心缓存
	CentralCache::GetInstance()->ReleaseListToCentralCache(index, start);
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	//慢开始模式：                        当前最大数量              最大申请上限
	size_t batchNum = min(_free_list[index]._MaxSize(), SizeClass::GetBatchNum(size));
	if (batchNum == _free_list[index]._MaxSize())
	{    //慢开始，翻倍
		_free_list[index]._MaxSize() <<= 1;
	}
	//批量申请
	void* start = nullptr;
	void* end = nullptr;
	size_t actualNum = CentralCache::GetInstance()->FetchRangeobj(batchNum, size, start, end);
	
	// 返回
	assert(actualNum > 0);
	if(actualNum == 1)//得到一个直接返回
        return start;
	else {            //得到一批，返回头，其他链入链表
		_free_list[index].Push(NextObject(start), end, actualNum - 1);
		NextObject(start) = nullptr;
		return start;
	}
}

