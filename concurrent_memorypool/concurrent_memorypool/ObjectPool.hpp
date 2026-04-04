#pragma once
#include <iostream>
#include <vector>

using std::cout;
using std::endl;

namespace ObjectPoolModule
{
	template<class T>
	class ObjectPool
	{
	public:
		const int ExpansionSize = 128 * 1024;
		T* New()
		{
			T* obj = nullptr;
			//释放链表中的空间可以复用
			if (_freelist)
			{
				obj = (T*)_freelist;
				_freelist = *((void**)_freelist);//把freelist指向的未知大小空间的前面指针部分提取出来（也就是下个节点
			}
			else {
				//空间不足，先扩容
				if (_remaining_memory < _object_size)
				{
					_memory = (char*)malloc(ExpansionSize);
					if (_memory == nullptr)
					{
						throw std::bad_alloc();
					}
					_remaining_memory += ExpansionSize;
				}
				//分配空间
				obj = (T*)_memory;
				_memory += _object_size;
				_remaining_memory -= _object_size;
			}
			return obj;
		}
		void Delete(T* del)
		{
			//头插法
			//把数据空间的前4/8个字节存放下个节点指针
			//把void*存进开头
			*((void**)del) = _freelist;
			_freelist = del;
		}

	private:
		//void* _memory;void*不知道其大小，不能用+-移动
		char* _memory = nullptr; //指向内存池未被用过的空间
		int _remaining_memory = 0;//未被使用的空间的剩余大小
		//int object_size = sizeof(T) T大小小于void*时，无法插入嵌入式释放链表
		int _object_size = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);//单个数据开辟空间大小
		void* _freelist = nullptr;//指向嵌入式释放链表
	};

		struct TreeNode
		{
			int _val;
			TreeNode* _left;
			TreeNode* _right;
	
			TreeNode()
				:_val(0)
				, _left(nullptr)
				, _right(nullptr)
			{}
		};
	
		void TestObjectPool()
		{
			// 申请释放的轮次
			const size_t Rounds = 5;
	
			// 每轮申请释放多少次
			const size_t N = 100000;
	
			std::vector<TreeNode*> v1;
			v1.reserve(N);
	
			size_t begin1 = clock();
			for (size_t j = 0; j < Rounds; ++j)
			{
				for (int i = 0; i < N; ++i)
				{
					v1.push_back(new TreeNode);
				}
				for (int i = 0; i < N; ++i)
				{
					delete v1[i];
				}
				v1.clear();
			}
	
			size_t end1 = clock();
	
			std::vector<TreeNode*> v2;
			v2.reserve(N);
	
			ObjectPool<TreeNode> TNPool;
			size_t begin2 = clock();
			for (size_t j = 0; j < Rounds; ++j)
			{
				for (int i = 0; i < N; ++i)
				{
					v2.push_back(TNPool.New());
				}
				for (int i = 0; i < N; ++i)
				{
					TNPool.Delete(v2[i]);
				}
				v2.clear();
			}
			size_t end2 = clock();
	
			cout << "new cost time:" << end1 - begin1 << endl;
			cout << "object pool cost time:" << end2 - begin2 << endl;
		}
}