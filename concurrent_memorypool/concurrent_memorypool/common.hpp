#pragma once

#include <iostream>
#include <vector>
#include <assert.h>

namespace CommonModule
{
	static const int FREE_LIST_BUCKETS_COUNT = 208;
	static const int MAX_SIZE = (1024 << 10);

	class FreeList
	{
	private:
		void*& NextObject(void* obj)
		{
			return *((void**)obj);
		}
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
			assert(size < MAX_SIZE);

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
