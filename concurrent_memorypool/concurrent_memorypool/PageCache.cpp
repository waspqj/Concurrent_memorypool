#include "PageCache.h"

PageCache PageCache::_instance;
//std::vector<SpanList> _nPage_list
//std::unordered_map<PAGE_ID, Span*> _span_map;

Span* PageCache::NewSpan(size_t k)
{
	if (k >= NPAGE_MAX)//大于NPAGE_MAX，直接向内存申请
	{
		std::cout<< "alloc big one from 内存" << std::endl;
		Span* bigSpan = new Span;
		void* ptr = SystemAlloc(k);
		bigSpan->_page_id = (PAGE_ID)ptr >> PAGE_SHIFT;
		bigSpan->_n = k;
		// 回收时同样依靠页号找到span
		_span_map[bigSpan->_page_id] = bigSpan;
		//_span_map[bigSpan->_page_id + bigSpan->_n - 1] = bigSpan;

		return bigSpan;
	}
	else {
		if (!_nPage_list[k].Empty())
		{
			return _nPage_list[k].Popfront();
		}
		//没有现成的span，从页数更大的span中拆分
		for (int npage = k + 1; npage < NPAGE_MAX; npage++)
		{
			if (!_nPage_list[npage].Empty())
			{
				std::cout << "split span" << npage << std::endl;
				Span* nSpan = _nPage_list[npage].Popfront();
				// 先拆前k页
				Span* kSpan = new Span;
				kSpan->_page_id = nSpan->_page_id;
				kSpan->_n = k;
				// 存储kspan 页号 和 span的映射关系
				for (int i = 0; i < k; ++i)
				{
					_span_map[kSpan->_page_id + i] = kSpan;
				}
				// 再拿出后 npage - k 页
				nSpan->_page_id += k;
				nSpan->_n -= k;
				// 存储nspan 首尾页号 和 span的映射关系 ？—— 这里是否可靠有待商榷
				_span_map[nSpan->_page_id] = nSpan;
				_span_map[nSpan->_page_id + nSpan->_n - 1] = nSpan;
				// 把后 npage - k 页存到 _nPage_list[npage - k]
				_nPage_list[nSpan->_n].Pushfront(nSpan);
				// 返回k页的span
				return kSpan;
			}
		}
		//没有大块可拆的span，从内存获取最大块span
		Span* bigSpan = new Span;
		void* ptr = SystemAlloc(NPAGE_MAX);
		std::cout << "alloc big span" << std::endl;
		bigSpan->_page_id = (PAGE_ID)ptr >> PAGE_SHIFT;
		bigSpan->_n = NPAGE_MAX - 1;
		// 存储bigspan 首尾页号 和 span的映射关系 ？—— 这里是否可靠有待商榷
		_span_map[bigSpan->_page_id] = bigSpan;
		_span_map[bigSpan->_page_id + bigSpan->_n - 1] = bigSpan;
		//放入_nPage_list[NPAGE_MAX - 1]
		_nPage_list[NPAGE_MAX - 1].Pushfront(bigSpan);
		//回调自己，递归获取k页的span
		return NewSpan(k);
	}
}

Span* PageCache::GetSpan(PAGE_ID page_id)
{
	auto it = _span_map.find(page_id);
	if(it == _span_map.end())
		return nullptr;
	return it->second;
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//std::cout << "ReleaseSpanToPageCache" << std::endl;
	if (span->_n >= NPAGE_MAX) //大于NPAGE_MAX,直接还给内存
	{
		std::cout << "Free a big one to 内存" << std::endl;
        SystemFree((void*)(span->_page_id << PAGE_SHIFT));
	}
	else {
		//尝试合并前后所有能合的span
		while (true)
		{      //找到前一页所在的span
			Span* prev = GetSpan(span->_page_id - 1);
			if (prev == nullptr)
				break;
			if (prev->_isusing == true)
				break;
			if (prev->_n + span->_n > NPAGE_MAX - 1)
				break;
			//合并
			std::cout << "合并front" << std::endl;
			span->_page_id -= prev->_n;
			span->_n += prev->_n;
			//删除prev
			_nPage_list[prev->_n].Pop(prev);
			delete prev;
		}
		while (true)
		{           //找到后一页所在的span
			Span* next = GetSpan(span->_page_id + span->_n);
			if (next == nullptr)
				break;
			if (next->_isusing == true)
				break;
			if (span->_n + next->_n > NPAGE_MAX - 1)
				break;
			std::cout << "合并back" << std::endl;
			//合并
			span->_n += next->_n;
			//删除next
			_nPage_list[next->_n].Pop(next);
			delete next;
		}
		//将合并后的span放入对应页数
		_nPage_list[span->_n].Pushfront(span);
		_span_map[span->_page_id] = span;
		_span_map[span->_page_id + span->_n - 1] = span;
	}
}
