#pragma once
#include "common.hpp"

using namespace CommonModule;

class PageCache
{
private:
	PageCache(const PageCache&) = delete;
    PageCache& operator=(const PageCache&) = delete;
	PageCache() :_nPage_list(NPAGE_MAX)
    {}
public:
	static PageCache *GetInstance()
	{
		if(_instance == nullptr)
        {
            _instance = new PageCache();
        }
		return _instance;
	}
	Span* NewSpan(size_t);
	Span* GetSpan(PAGE_ID);

	void ReleaseSpanToPageCache(Span* span);
private:
	std::vector<SpanList> _nPage_list;
	//页号和所在span的映射
	std::unordered_map<PAGE_ID, Span*> _span_map;

	static PageCache* _instance;
public:
	std::mutex _mtx;
};