#include "PageCache.hpp"

PageCache PageCache::_sInst;

// 获取一个k页的span
Span *PageCache::NewSpan(size_t k)
{
    assert(k > 0);
    // 大于128page直接向堆申请
    if (k > NPAGE - 1)
    {
        void *ptr = SystemAlloc(k);
        // Span* span = new Span;
        Span *span = _spanPool.New();
        span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
        span->_nPage = k;
        //_idSpanMap[span->_pageId] = span;
        _idSpanMap.set(span->_pageId, span);
        return span;
    }
    // 先检查第k个桶里面有没有span
    if (!_spanLists[k].Empty())
    {
        Span *kSpan = _spanLists[k].PopFront();

        // 建立id和span的映射关系，方便central cache回收小块内存时，查找对应despan
        for (PAGE_ID i = 0; i < kSpan->_nPage; ++i)
        {
            // _idSpanMap[kSpan->_pageId + i] = kSpan;
            _idSpanMap.set(kSpan->_pageId + i, kSpan);
        }
        return kSpan;
    }

    // 检查一下后面的桶里面有没有span，如果有可以把它进行切分
    for (size_t i = k + 1; i < NPAGE; ++i)
    {
        if (!_spanLists[i].Empty())
        {
            Span *nSpan = _spanLists[i].PopFront();
            // Span *kSpan = new Span;
            Span *kSpan = _spanPool.New();

            // 在nspan的头部切一个k页下来
            // k页span返回
            // nspan再挂到对应映射的位置
            kSpan->_pageId = nSpan->_pageId;
            kSpan->_nPage = k;

            nSpan->_pageId += k;
            nSpan->_nPage -= k;

            _spanLists[nSpan->_nPage].PushFront(nSpan);

            // 存储nSpan的首位页号跟nspan映射，方便page cache回收内存时，进行合并查找
            // _idSpanMap[nSpan->_pageId] = nSpan;
            // _idSpanMap[nSpan->_pageId + nSpan->_nPage - 1] = nSpan;
            _idSpanMap.set(nSpan->_pageId, nSpan);
            _idSpanMap.set(nSpan->_pageId + nSpan->_nPage - 1, nSpan);

            // 建立id和span的映射关系，方便central cache回收小块内存时，查找对应的span
            for (PAGE_ID i = 0; i < kSpan->_nPage; ++i)
            {
                // _idSpanMap[kSpan->_pageId + i] = kSpan;
                _idSpanMap.set(kSpan->_pageId + i, kSpan);
            }

            return kSpan;
        }
    }
    // 到这说明后面没有更大的span了，这时直接找堆要一个128页的span
    // Span *bigSpan = new Span;
    Span *bigSpan = _spanPool.New();
    void *ptr = SystemAlloc(NPAGE - 1);
    bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
    bigSpan->_nPage = NPAGE - 1;

    _spanLists[bigSpan->_nPage].PushFront(bigSpan);

    return NewSpan(k);
}

Span *PageCache::MapObjectToSpan(void *obj)
{
    PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);
    // std::unique_lock<std::mutex> lock(_pageMutex);
    // auto ret = _idSpanMap.find(id);
    // if (ret != _idSpanMap.end())
    // {
    //     return ret->second;
    // }
    // else
    // {
    //     assert(false);
    //     return nullptr;
    // }
    auto ret = (Span *)_idSpanMap.get(id);
    assert(ret != nullptr);
    return ret;
}

void PageCache::ReleaseSpanToPageCache(Span *span)
{
    // 大于128page的直接还给堆
    if (span->_nPage > NPAGE - 1)
    {
        void *ptr = (void *)(span->_pageId << PAGE_SHIFT);
        SystemFree(ptr);
        // delete span;
        _spanPool.Delete(span);
        return;
    }

    // 对span前后的页，尝试合并，缓解内存碎片的问题
    while (1)
    {
        PAGE_ID prevId = span->_pageId - 1;
        // auto ret = _idSpanMap.find(prevId);
        // // 前面文档页号没有，进行合并
        // if (ret == _idSpanMap.end())
        // {
        //     break;
        // }
        auto ret = (Span *)_idSpanMap.get(prevId);
        if (ret == nullptr)
        {
            break;
        }
        // 前面相邻页的span在使用，不进行合并
        // Span *prevSpan = ret->second;
        Span *prevSpan = ret;
        if (prevSpan->_isUse == true)
        {
            break;
        }

        // 合并出超过128页的span没办法管理，不合并
        if (prevSpan->_nPage + span->_nPage > NPAGE - 1)
        {
            break;
        }

        span->_pageId = prevSpan->_pageId;
        span->_nPage += prevSpan->_nPage;

        _spanLists[prevSpan->_nPage].Erase(prevSpan);
        _spanPool.Delete(prevSpan);
        // delete prevSpan;
        // 注意了：span对象是new出来的，span对象不直接存储内存空间，span对象
        // 通过页号来与内存空间建立映射关系，因此当prevSpan所指向的span对象无用时应该delete掉
    }

    // 向后合并
    while (1)
    {
        PAGE_ID nextId = span->_pageId + span->_nPage;
        // auto ret = _idSpanMap.find(nextId);
        // if (ret == _idSpanMap.end())
        // {
        //     break;
        // }
        auto ret = (Span *)_idSpanMap.get(nextId);
        if (ret == nullptr)
        {
            break;
        }
        // Span *nextSpan = ret->second;
        Span *nextSpan = ret;
        if (nextSpan->_isUse == true)
        {
            break;
        }

        if (nextSpan->_nPage + span->_nPage > NPAGE - 1)
        {
            break;
        }
        span->_nPage += nextSpan->_nPage;

        _spanLists[nextSpan->_nPage].Erase(nextSpan);
        _spanPool.Delete(nextSpan);
        // delete nextSpan;
    }
    _spanLists[span->_nPage].PushFront(span);
    span->_isUse = false;
    // _idSpanMap[span->_pageId] = span;
    // _idSpanMap[span->_pageId + span->_nPage-1] = span;
    _idSpanMap.set(span->_pageId, span);
    _idSpanMap.set(span->_pageId + span->_nPage - 1, span);
}