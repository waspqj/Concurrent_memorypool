//#include "ObjectPool.hpp"
//using namespace ObjectPoolModule;
#include "ThreadCache.h"
#include <thread>

void* my_malloc(size_t size)
{
	//std::cout << std::this_thread::get_id() << ": " << ThreadCache::GetInstance() <<std::endl;
	return ThreadCache::GetInstance()->Allocate(size);
}
void my_free(void* ptr, size_t size)
{
	ThreadCache::GetInstance()->Deallocate(ptr, size);
}

void thread_func()
{
	for (int i = 0; i < 10; ++i)
	{
		int *p = (int*)my_malloc(sizeof(int));
		//p[0] = 0;
	}
}

void TestForTLSThreadCache()
{
	std::thread t1(thread_func);
	std::thread t2(thread_func);
	t1.join();
	t2.join();
}
void TestEroment()
{
	std::cout << sizeof(int*) << std::endl;
	std::cout << sizeof(PAGE_ID) << std::endl;
	
}

void TestConcurrentMemoryPool()
{
	for (int i = 0; i < 1024; ++i)
	{
		my_malloc(1);
		//std::cout << i << ": " << std::endl;
	}
	my_malloc(8);
}
int main()
{
	//TestObjectPool();
	//TestForTLSThreadCache();
	//ThreadCache::GetInstance();

	TestConcurrentMemoryPool();
	return 0;
}