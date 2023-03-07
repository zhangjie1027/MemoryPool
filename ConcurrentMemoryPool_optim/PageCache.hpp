#pragma once

#include "Common.hpp"
#include "ObjectPool.hpp"

class PageCache
{
private:
    SpanList _spanLists[NPAGE];
    ObjectPool<Span> _spanPool; // 使用定长内存池代替new
    static PageCache _sInst;
    std::unordered_map<PAGE_ID, Span *> _idSpanMap; // 页号与span之间的映射关系，当thread cache将一部分对象回收给central caceh，这群对象可以通过这个映射关系找到对应的span

public:
    std::mutex _pageMutex; // 全锁

public:
    static PageCache *GetInstance()
    {
        return &_sInst;
    }

    // 获取一个k页的span
    Span *NewSpan(size_t k);

    // 从对象到span的映射
    Span *MapObjectToSpan(void *obj);

    // 释放空闲的span回到PageCache，并合并相邻的span
    void ReleaseSpanToPageCache(Span *span);

private:
    PageCache() {}
    PageCache(const PageCache &) = delete;
};