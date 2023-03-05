#pragma once

#include "Common.hpp"

class PageCache
{
private:
    SpanList _spanLists[NPAGE];
    static PageCache _sInst;

public:
    std::mutex _pageMutex; // 全锁

public:
    static PageCache *GetInstance()
    {
        return &_sInst;
    }

    // 获取一个k页的span
    Span *NewSpan(size_t k);

private:
    PageCache() {}
    PageCache(const PageCache &) = delete;
};