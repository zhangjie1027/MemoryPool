#pragma once
#include "Common.hpp"
#include "ThreadCache.hpp"

static void *ConcurrentAlloc(size_t size)
{
    // 通过TLS 每个线程无锁的获取专属于自己的ThreadCache对象
    if (pTLSThreadCache == nullptr)
    {
        pTLSThreadCache = new ThreadCache;
    }
    std::cout << "线程id:" << std::this_thread::get_id() << "-->TLS" << pTLSThreadCache << std::endl;

    return pTLSThreadCache->Alloccate(size);
}

static void ConcurrentFree(void *ptr, size_t size)
{
    assert(pTLSThreadCache);
    pTLSThreadCache->Deallocate(ptr, size);
}