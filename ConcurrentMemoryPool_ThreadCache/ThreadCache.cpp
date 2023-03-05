#include "ThreadCache.hpp"

void *ThreadCache::Alloccate(size_t size)
{
    assert(size <= MAX_BYTES);
    size_t alignSize = SizeClass::RoundUp(size); // 计算出内存块的对其大小alignSize，即要获取的单个obj的大小
    size_t index = SizeClass::Index(size);       // 内存块所在的自由链表的下标

    if (!_freeLists[index].Empty()) // 自由链表中有对象，则直接Pop一个返回
    {
        return _freeLists[index].Pop();
    }
    else // 没有，则从CentralCache中获取一定数量的对象，插入到自由链表中并返回一个对象
    {
        return FetchFromCentralCache(index, alignSize);
    }
}

void ThreadCache::Deallocate(void *ptr, size_t size)
{
    assert(ptr);
    assert(size <= MAX_BYTES);

    // 找对映射的自由链表，将对象插入
    size_t index = SizeClass::Index(size);
    _freeLists[index].Push(ptr);
}

void *ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
    //.............
    return nullptr;
}