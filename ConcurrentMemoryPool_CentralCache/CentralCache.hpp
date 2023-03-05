#pragma once

#include "Common.hpp"

// 单例模式
class CentralCache
{
private:
    static CentralCache _sInst;
    SpanList _spanLists[NFREELIST];

public:
    static CentralCache *GetInstance()
    {
        return &_sInst;
    }

    // 获取一个非空的span
    Span *GetOneSpan(SpanList &list, size_t byte_size);

    // 从中心缓存获取一定数量的对象给thread cache
    size_t FetchRangeObj(void *&start, void *&end, size_t batchNum, size_t size);

private:
    CentralCache() {}

    CentralCache(const CentralCache &) = delete;
};