//#include "ObjectPool.hpp"
//using namespace ObjectPoolModule;
#include "ThreadCache.h"
#include <thread>

void* my_malloc(size_t size)
{
	std::cout << std::this_thread::get_id() << "_______: ";
	void* ptr = ThreadCache::GetInstance()->Allocate(size);
	std::cout << ptr << std::endl;
	return ptr;
}
void my_free(void* ptr, size_t size)
{
	std::cout << std::this_thread::get_id() << "~~~~~~: " << ptr<< std::endl;
	ThreadCache::GetInstance()->Deallocate(ptr, size);
}

void thread_func1()
{
	std::vector<void* > ps;
	for (int i = 0; i < 7; ++i)
	{
		void *p = my_malloc(8);
		ps.push_back(p);
	}
    for (auto p : ps)
        my_free(p, 8);
}
void thread_func2()
{
    std::vector<void* > ps;
	for (int i = 0; i < 7; ++i)
	{
		void* p = my_malloc(16);
		ps.push_back(p);
		//Sleep(3);
	}
    for (auto p : ps)
        my_free(p, 16);
}
void TestForTLSThreadCache()
{
	std::thread t2(thread_func2);
	std::thread t1(thread_func1);
	t1.join();
	t2.join();
}
void TestEroment()
{
	std::cout << sizeof(int*) << std::endl;
	std::cout << sizeof(PAGE_ID) << std::endl;
	
}

void TestConcurrentAlloc()
{
	std::vector<void*> ps;
	for (int i = 0; i <= 1024; ++i)
	{
		std::cout << i;
		void *ptr = my_malloc(8);
		ps.push_back(ptr);
	}
	for(auto p : ps)
        my_free(p, 8);
	//my_malloc(8);
}
void TestConcurrentFree()
{
	void* p1 = my_malloc(1);
	void* p2 = my_malloc(4);
	void* p3 = my_malloc(8);
	void* p4 = my_malloc(3);
	void* p5 = my_malloc(6);
	void* p6 = my_malloc(5);
	void* p7 = my_malloc(5);
	//void* p8 = my_malloc(5);

	my_free(p1, 1);
	my_free(p2, 4);
	my_free(p3, 8);
	my_free(p4, 3);
	my_free(p5, 6);
	my_free(p6, 5);
	my_free(p7, 5);
	//my_free(p8, 5);

}
int main()
{
	//TestObjectPool();
	TestForTLSThreadCache();
	//ThreadCache::GetInstance();

	//TestConcurrentAlloc();
	//TestConcurrentFree();
	return 0;
}