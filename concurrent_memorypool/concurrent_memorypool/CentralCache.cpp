#include "CentralCache.h"

CentralCache* CentralCache::_instance;
//std::vector<SpanList> _span_lists;

Span* CentralCache::FetchOneSpan(SpanList& list, size_t size)
{
	Span* pos = list.Begin();
	while (pos != list.End())
	{   
		if (pos->_freelist != nullptr)//得到有效的span
			return pos;
		else 
			pos = pos->_next;
	}
	if (list._mtx.try_lock() == true)
	{
		int a = 10;
	}
	//解开centralcache的桶锁，支持其他线程同时释放内存对象
	list._mtx.unlock();
	// 对从页缓存获取span的行为加锁
	PageCache::GetInstance()->_mtx.lock();
	//没有可用span, 获取一个span
	pos = PageCache::GetInstance()->NewSpan(SizeClass::GetnPage(size));
    pos->_size = size;
	pos->_isusing = true;

	if (PageCache::GetInstance()->_mtx.try_lock() == true)
	{
		int a = 10;
	}
	PageCache::GetInstance()->_mtx.unlock();

	//使用前切分大块span
	char* start = (char*)(pos->_page_id << PAGE_SHIFT);
	char* end = start + (pos->_n << PAGE_SHIFT);
	//先切一块用来尾插
	pos->_freelist = start;
	NextObject(start) = nullptr;
	char* tail = start;
	start += pos->_size;
	//剩余的切分
	while (start < end)
	{
		NextObject(tail) = start;
        tail = start;
        NextObject(tail) = nullptr;
        start += pos->_size;
	}
	//以上部分span还没进入centralcache，不急加锁
	list._mtx.lock();

	//把切分好的span插入span_list
    list.Insert(pos, list.Begin());
	//返回span
	return pos;
}
size_t CentralCache::FetchRangeobj(size_t batchNum, size_t size, void*& begin, void*& end)
{
    size_t index = SizeClass::Index(size);

	SpanList& list = _span_lists[index];
	//加上桶锁
	list._mtx.lock();
	//获取一个pos
	Span* pos = FetchOneSpan(list, size);
	assert(pos);
	assert(pos->_freelist);

	begin = pos->_freelist;
	end = begin;
	int i = 1;           //可能不够 
	while (i < batchNum && NextObject(end))
	{
		end = NextObject(end);
		++i;
	}
    pos->_useCount += i;
	//pos->_isusing = true;
	pos->_freelist = NextObject(end);
	NextObject(end) = nullptr;

	if (list._mtx.try_lock() == true)
	{
		int a = 10;
	}

	list._mtx.unlock();
	return i;//返回实际获取的个数
}

void CentralCache::ReleaseListToCentralCache(size_t index, void* start)
{
	//遍历回收链表，逐一还给对应span(都在同一个桶里）
	while (start)
	{
		Span* span = PageCache::GetInstance()->GetSpan(SizeClass::GetPageId(start));
		//加锁
		_span_lists[index]._mtx.lock();
		//头插到freelist
		void* next = NextObject(start);
        NextObject(start) = span->_freelist;
        span->_freelist = start;
		// 写错了，大傻春
		//void* tmp = start;
		//NextObject(tmp) = span->_freelist;
  //      span->_freelist = tmp;
		
		span->_useCount--;
		//没人使用这个span，交还给PageCache
        if (span->_useCount == 0)
        {
			_span_lists[index].Pop(span);
			span->_freelist = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//span->_isusing = false;不能在这里，必须和span的释放加锁绑定

			if (_span_lists[index]._mtx.try_lock() == true)
			{
				int a = 10;
			}
			//暂时释放桶锁
			_span_lists[index]._mtx.unlock();
			//锁住PageCache
            PageCache::GetInstance()->_mtx.lock();
            PageCache::GetInstance()->ReleaseSpanToPageCache(span);
			if (PageCache::GetInstance()->_mtx.try_lock() == true)
			{
				int a = 10;
			}
            PageCache::GetInstance()->_mtx.unlock();

			_span_lists[index]._mtx.lock();
        }
		if (_span_lists[index]._mtx.try_lock() == true)
		{
			int a = 10;
		}
		//释放锁
        _span_lists[index]._mtx.unlock();

		start = next;
	}
}

