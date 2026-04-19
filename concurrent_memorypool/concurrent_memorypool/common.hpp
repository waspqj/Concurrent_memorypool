#pragma once

#include <iostream>
#include <vector>
#include <mutex>
#include <assert.h>

#ifdef _WIN64
	typedef unsigned long long PAGE_ID;
#elif _WIN32
	typedef size_t PAGE_ID;
#endif

namespace CommonModule
{
	static const int FREE_LIST_BUCKETS_COUNT = 208;
	static const int MAX_BYTES = (1024 << 10);

	void*& NextObject(void* obj)
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
		}
		void* Pop()
		{
			assert(_head);

			void* obj = _head;
			_head = NextObject(_head);
			return _head;
		}
		inline bool Empty()
		{
			return (_head == nullptr);
		}
	private:
		void* _head;
	};

	struct Span
	{ 
		PAGE_ID _page_id = 0; //span起始页的id
		size_t _n = 0; //页数

		Span* _prev; // 双向链表的结构
        Span* _next;

		size_t _useCount = 0; // 分配给threadcache的小对象数量
		void* _freelist = nullptr; // 切好的小块内存的链表

	};

	class SpanList
	{
	public:
		SpanList()
		{
			_head = new Span();
			_head->_prev = _head;
            _head->_next = _head;
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

		void Pop(Span* span)
		{
            assert(span);
			assert(span != _head);

			Span* prev = span->_prev;
			Span* next = span->_next;
			prev->_next = next;
            next->_prev = prev;
		}
	private:
		Span* _head; // 双向链表头

	public:
		std::mutex _mtx;//桶锁
	};


	// 整体控制在最多10%左右的内碎⽚浪费
	 // [1,128] 8byte对⻬ freelist[0,16)
	 // [128+1,1024] 16byte对⻬ freelist[16,72)
	 // [1024+1,81024] 128byte对⻬ freelist[72,128)
	//  [8*1024+1,641024] 1024byte对⻬ freelist[128,184)
	//  [64*1024+1,256*1024] 8*1024byte对⻬ freelist[184,208)
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
			{
				return _RoundUp(size, 8);
			}
			else if (size <= 1024)
			{
				return _RoundUp(size, 16);
			}
			else if (size <= (1024 << 3))
			{
				return _RoundUp(size, 128);
			}
			else if (size <= (1024 << 6))
			{
				return _RoundUp(size, 1024);
			}
			else if (size <= (1024 << 8))
			{
				return _RoundUp(size, (1024 << 3));
			}
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
		}
	};
}
