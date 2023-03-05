#include "CentralCache.hpp"
#include "PageCache.hpp"

CentralCache CentralCache::_sInst;

// 获取一个非空的span
Span *CentralCache::GetOneSpan(SpanList &list, size_t size)
{
    // 查看当前的spanList中是否还有未分配对象的span
    Span *it = list.Begin();
    while (it != list.End())
    {
        if (it->_freeList != nullptr)
        {
            return it;
        }
        else
        {
            it = it->_next;
        }
    }

    // 先把central cache的桶锁解掉，这样其他线程释放对象回来，不会阻塞
    list._mutex.unlock();

    // 走到这说明没有空闲span了，只能找page cache要
    PageCache::GetInstance()->_pageMutex.lock(); // 全锁
    Span *span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
    PageCache::GetInstance()->_pageMutex.unlock();

    // 对获取的span进行切分，不需要加锁，因为这会其他线程还访问不到这个span（span是局部变量）
    // 计算span的大内存的起始地址和大内存的大小（字节数）
    char *start = (char *)(span->_pageId << PAGE_SHIFT);
    size_t bytes = span->_nPage << PAGE_SHIFT;
    char *end = start + bytes;

    // 把大内存切成自由链表链接起来
    // 先切一块下来做头，方便尾插
    span->_freeList = start;
    start += size;
    void *tail = span->_freeList;
    int i = 1;
    while (start < end)
    {
        ++i;
        NextObj(tail) = start;
        tail = NextObj(tail);
        start += size;
    }

    // 切好span以后，需要把span挂到桶里面去的时候，再加锁
    list._mutex.lock();
    list.PushFront(span);

    return span;
}

// 从中心缓存获取一定数量的对象给thread cache
size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t bacthNum, size_t size)
{
    size_t index = SizeClass::Index(size); // 先根据对象的大小找到_spanLists对应的下标
    _spanLists[index]._mutex.lock();       // 寻找合适的span

    Span *span = GetOneSpan(_spanLists[index], size);
    assert(span);
    assert(span->_freeList);

    // 从span中获取batchNum个对象
    // 如果不够batchNum个，有多少拿多少
    start = span->_freeList;
    end = start;
    size_t i = 0;
    size_t actualNum = 1;
    while (i < bacthNum - 1 && NextObj(end) != nullptr)
    {
        end = NextObj(end);
        ++i;
        ++actualNum;
    }
    span->_freeList = NextObj(end);
    NextObj(end) = nullptr;

    _spanLists[index]._mutex.unlock();

    return actualNum;
}