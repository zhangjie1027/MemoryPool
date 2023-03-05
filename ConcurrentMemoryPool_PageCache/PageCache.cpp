#include "PageCache.hpp"

PageCache PageCache::_sInst;

// 获取一个k页的span
Span *PageCache::NewSpan(size_t k)
{
    assert(k > 0 && k < NPAGE);
    // 先检查第k个桶里面有没有span
    if (!_spanLists[k].Empty())
    {
        return _spanLists[k].PopFront();
    }

    // 检查一下后面的桶里面有没有span，如果有可以把它进行切分
    for (size_t i = k + 1; i < NPAGE; ++i)
    {
        if (!_spanLists[i].Empty())
        {
            Span *nSpan = _spanLists[i].PopFront();
            Span *kSpan = new Span;

            // 在nspan的头部切一个k页下来
            // k页span返回
            // nspan再挂到对应映射的位置
            kSpan->_pageId = nSpan->_pageId;
            kSpan->_nPage = k;

            nSpan->_pageId += k;
            nSpan->_nPage -= k;

            _spanLists[nSpan->_nPage].PushFront(nSpan);

            return kSpan;
        }
    }
    // 到这说明后面没有更大的span了，这时直接找堆要一个128页的span
    Span *bigSpan = new Span;
    void *ptr = SystemAlloc(NPAGE - 1);
    bigSpan->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
    bigSpan->_nPage = NPAGE - 1;

    _spanLists[bigSpan->_nPage].PushFront(bigSpan);

    return NewSpan(k);
}