#pragma once

#include <iostream>

#include <vector>
#include <unordered_map>

#include <assert.h>
#include <mutex>

#include <algorithm>

#include <windows.h>
#include <atomic>
#include <thread>

#include "ObjectPool.hpp"


//#ifdef _WIN64
//#pragma message("_WIN64 is defined")
//#endif
//#ifdef _WIN32
//#pragma message("_WIN32 is defined")
//#endif
// 因为64位系统，_WIN64和_WIN32同时定义
//我这里的条件编译为什么没有生效，我试过调换_WIN64和_WIN32的位置，结果都是编译第一条

namespace CommonModule
{
	using std::cout;
	using std::endl;
	using namespace ObjectPoolModule;

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#endif

	static const int FREE_LIST_BUCKETS_COUNT = 208;
	static const int  NPAGE_MAX = 129; // 128页
	static const int MAX_BYTES = (256 << 10);// 256kb，32页
    static const int PAGE_SHIFT = 13; //(PAGE_id and void*) 页大小,8kb, 8*1024 , 2^(3*10)
										
	inline static void* SystemAlloc(size_t kpage)
	{
#ifdef _WIN32
		void* ptr = VirtualAlloc(0, kpage * (1 << PAGE_SHIFT),
			MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
		// brk mmap等
#endif
		if (ptr == nullptr)
			throw std::bad_alloc();
		return ptr;
	}
    static void* SystemFree(void* ptr)
	{
#ifdef _WIN32
		VirtualFree(ptr, 0, MEM_RELEASE);
#else
		// brk mmap等
#endif
		return nullptr;
	}

	static void*& NextObject(void* obj)
	{
		return *((void**)obj);
	}

	class FreeList
	{
	public:
		void Push(void* obj)
		{
			assert(obj);
			
			NextObject(obj) = _head;
			_head = obj;
			++_size;
		}
		void Push(void* start, void* end, size_t size)
		{
            assert(start);
            assert(end);

			NextObject(end) = _head;
            _head = start;
            _size += size;
		}
		void* Pop()
		{
			assert(_head);

			void* obj = _head;
			_head = NextObject(_head);
            --_size;

			return obj;
		}
		size_t Pop(size_t len, void*& start, void*& end)
		{
			assert(_size >= len);
			start = _head;
			end = start;
			int i = 1;
			while(i < len)
			{
				end = NextObject(end);
				++i;
			}
			_head = NextObject(end);
			NextObject(end) = nullptr;

            _size -= len;
			return i;
		}
		inline bool Empty()
		{
			return (_head == nullptr);
		}
		size_t _Size()
		{
			return _size;
		}
		size_t& _MaxSize()
		{
			return _maxSize;
		}
	private:
		void* _head = nullptr;
		size_t _size = 0;
		//慢开始策略
		size_t _maxSize = 1;
	};

	struct Span
	{ 
		PAGE_ID _page_id = 0; //span起始页的id
		size_t _n = 0; //页数

		Span* _prev = nullptr; // 双向链表的结构
        Span* _next = nullptr;

		size_t _useCount = 0; // 分配给threadcache的小对象数量
		void* _freelist = nullptr; // 切好的小块内存的链表

		size_t _size = 0; // 小对象大小
		bool _isusing = false; // span是否正在使用：真正的含义是是否分配到threadcache中
	};

	class SpanList
	{
	public:
		SpanList()
		{
			_head = new Span;
			_head->_prev = _head;
            _head->_next = _head;
		}

		Span* Begin()
		{
			return _head->_next;
		}
        Span* End()
        {
            return _head;
        }
        void Insert(Span* newspan, Span* pos)
        {
			assert(newspan);

			Span* prev = pos->_prev;
			prev->_next = newspan;
			newspan->_prev = prev;
			newspan->_next = pos;
			pos->_prev = newspan;
        }
		void Pushfront(Span* span)
		{
			Insert(span, Begin());
		}
		void Pop(Span* span)
		{
            assert(span);
			assert(span != _head);

			Span* prev = span->_prev;
			Span* next = span->_next;
			prev->_next = next;
            next->_prev = prev;
		}
		Span* Popfront()
		{
			Span* ret = Begin();
			Pop(ret);
			return ret;
		}
		bool Empty()
        {
            return (_head->_next == _head);
        }
	private:
		Span* _head; // 双向链表头

	public:
		std::mutex _mtx;//桶锁
	};


	// 整体控制在最多10%左右的内碎?浪费
	 // [1,128] 8byte对? freelist[0,16)
	 // [128+1,1024] 16byte对? freelist[16,72)
	 // [1024+1,81024] 128byte对? freelist[72,128)
	//  [8*1024+1,641024] 1024byte对? freelist[128,184)
	//  [64*1024+1,256*1024] 8*1024byte对? freelist[184,208)
	class SizeClass
	{
	public:
		static inline size_t _RoundUp(size_t size, size_t align)
		{
			return (size + align - 1) & ~(align - 1);
		}
		static inline size_t RoundUp(size_t size)
		{
			if (size <= 128)
				return _RoundUp(size, 8);
			else if (size <= 1024)
				return _RoundUp(size, 16);
			else if (size <= (1024 << 3))
				return _RoundUp(size, 128);
			else if (size <= (1024 << 6))
				return _RoundUp(size, 1024);
			else if (size <= (1024 << 8))
				return _RoundUp(size, (1024 << 3));
			else // 大对象按页对齐
				return _RoundUp(size, ((1 << PAGE_SHIFT) - 1));
		}

		static inline size_t _Index(size_t size, size_t align_shift)
		{
			return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
		}
		static inline size_t Index(size_t size)
		{
			assert(size < MAX_BYTES);

			static int array[4] = { 16, 56, 56, 56 };
			if (size <= 128)//	[0, 16)
			{
				return _Index(size, 3);
			}
			else if (size <= 1024)//  [16, 72)
			{
				return _Index(size - 128, 4) + array[0];
			}
			else if (size <= (1024 << 3))
			{
				return _Index(size - 1024, 7) + array[0] + array[1];
			}
			else if (size <= (1024 << 6))
			{
				return _Index(size - (1024 << 3), 10) + array[0] + array[1] + array[2];
			}
			else if (size <= (1024 << 8))
			{
				return _Index(size - (1024 << 6), 13) + array[0] + array[1] + array[2] + array[3];
			}
			return 0;
		}

		static inline size_t GetBatchNum(size_t size)
        { 
			assert(size > 0);
			size_t retNum = MAX_BYTES / size;

			// 限制批量申请内存的个数
			if(retNum == 0)
                retNum = 1;
			if (retNum < 2)
				retNum = 2;
			if(retNum > 512)
                retNum = 512;

			return retNum;
        }	
        static inline size_t GetnPage(size_t size)
        {
			size_t npage = (size * GetBatchNum(size)) >> PAGE_SHIFT;
            if(npage == 0)
                npage = 1;

			return npage;
        }
		static inline PAGE_ID GetPageId(void* ptr)
		{
			return ((PAGE_ID)ptr >> PAGE_SHIFT);
		}
	};
}
