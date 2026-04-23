#pragma once
#include "common.hpp"
#include "PageMap.h"

using namespace CommonModule;

class PageCache
{
private:
	PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;
	PageCache()
		:_nPage_list(NPAGE_MAX)
    {}

	//Span* _GetSpan(PAGE_ID page_id);
public:
	static PageCache *GetInstance()
	{
		return &_instance;
	}
	Span* NewSpan(size_t);
	Span* GetSpan(PAGE_ID);

	void ReleaseSpanToPageCache(Span* span);
private:
	std::vector<SpanList> _nPage_list;
	//页号和所在span的映射
	//std::unordered_map<PAGE_ID, Span*> _span_map;
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _span_map;
public:
	std::mutex _mtx;

	static PageCache _instance;
	ObjectPool<Span> spanPool;
};