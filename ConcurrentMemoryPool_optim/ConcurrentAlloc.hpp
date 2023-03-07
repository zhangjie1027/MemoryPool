#pragma once
#include "Common.hpp"
#include "ThreadCache.hpp"
#include "PageCache.hpp"
#include "ObjectPool.hpp"

static void *ConcurrentAlloc(size_t size)
{
    if (size > MAX_BYTES)
    {
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kPage = alignSize >> PAGE_SHIFT;

        PageCache::GetInstance()->_pageMutex.lock();
        Span *span = PageCache::GetInstance()->NewSpan(kPage);
        span->_objSize = alignSize;
        PageCache::GetInstance()->_pageMutex.unlock();

        void *ptr = (void *)(span->_pageId << PAGE_SHIFT);
        return ptr;
    }
    else
    {
        // 通过TLS 每个线程无锁的获取专属于自己的ThreadCache对象
        if (pTLSThreadCache == nullptr)
        {
            // pTLSThreadCache = new ThreadCache;
            static ObjectPool<ThreadCache> tcPool;
            pTLSThreadCache = tcPool.New();
        }
        // std::cout << "线程id:" << std::this_thread::get_id() << "-->TLS" << pTLSThreadCache << std::endl;

        return pTLSThreadCache->Alloccate(size);
    }
}

static void ConcurrentFree(void *ptr)
{
    Span *span = PageCache::GetInstance()->MapObjectToSpan(ptr);
    size_t size = span->_objSize;

    if (size > MAX_BYTES)
    {
        PageCache::GetInstance()->_pageMutex.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_pageMutex.unlock();
    }
    else
    {
        assert(pTLSThreadCache);
        pTLSThreadCache->Deallocate(ptr, size);
    }
}