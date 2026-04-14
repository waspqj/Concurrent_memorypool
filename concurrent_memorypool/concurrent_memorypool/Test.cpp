#include "ObjectPool.hpp"
#include "ThreadCache.h"
#include <thread>
using namespace ObjectPoolModule;

void* my_malloc(size_t size)
{
	std::cout << std::this_thread::get_id() << ": " << ThreadCache::GetInstance() <<std::endl;
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
	t1.join();
	std::thread t2(thread_func);
    t2.join();
}

int main()
{
	//TestObjectPool();
	TestForTLSThreadCache();
	ThreadCache::GetInstance();
	return 0;
}