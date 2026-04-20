#pragma once

#include "common.hpp"
#include "PageCache.h"

using namespace CommonModule;

class CentralCache 
{
private:
	CentralCache(const CentralCache&) = delete;
    CentralCache& operator=(const CentralCache&) = delete;
	CentralCache()
		:_span_lists(FREE_LIST_BUCKETS_COUNT)
	{}
public:
	static CentralCache *GetInstance()
	{
		if(_instance == nullptr)
            _instance = new CentralCache();
		return _instance;
	}
	Span* FetchOneSpan(SpanList &, size_t);
	size_t FetchRangeobj(size_t batchNum, size_t size, void*&, void*&);

	void ReleaseListToCentralCache(size_t index, void* start);
private:
	std::vector<SpanList> _span_lists;

	static CentralCache *_instance;
};