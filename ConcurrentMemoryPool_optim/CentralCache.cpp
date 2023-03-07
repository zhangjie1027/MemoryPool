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
    span->_isUse = true;
    span->_objSize = size;
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

    NextObj(tail) = nullptr;

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
    span->_useCount += actualNum;

    _spanLists[index]._mutex.unlock();

    return actualNum;
}

// 将一定数量的对象还给对应的span
void CentralCache::ReleaseListToSpans(void *start, size_t size)
{
    size_t index = SizeClass::Index(size);
    _spanLists[index]._mutex.lock(); // 桶锁
    while (start)
    {
        void *next = NextObj(start);
        // 把start开头的这一串自由链表内存还给属于它的span，一次循环还一个，一直还
        Span *span = PageCache::GetInstance()->MapObjectToSpan(start);
        NextObj(start) = span->_freeList;
        span->_freeList = start;
        span->_useCount--;

        // 说明span的切分出去的所有小内存块都回来了
        // 这个span可以回收给page cache，尝试前后页合并
        if (span->_useCount == 0)
        {
            _spanLists[index].Erase(span);
            span->_freeList = nullptr;
            span->_next = nullptr;
            span->_prev = nullptr;

            // 释放span给page cache，使用page cache的锁就可以
            // 解掉桶锁
            _spanLists[index]._mutex.unlock();

            PageCache::GetInstance()->_pageMutex.lock();
            PageCache::GetInstance()->ReleaseSpanToPageCache(span);
            PageCache::GetInstance()->_pageMutex.unlock();

            _spanLists[index]._mutex.lock();
        }
        start = next;
    }
    _spanLists[index]._mutex.unlock();
}