#pragma once

#include "common.hpp"

using namespace CommonModule;

class CentralCache 
{
public:
	CentralCache()
		:_span_lists(FREE_LIST_BUCKETS_COUNT)
	{}
private:
	std::vector<SpanList> _span_lists;
};