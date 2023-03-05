#include "ThreadCache.hpp"
#include "CentralCache.hpp"

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
    // 慢开始反馈调节算法
    // 1、最开始不会一次向central cache一次批量要太多，因为要太多了可能用不完
    // 2、如果你不要这个size大小内存需求，那么batchNum就会不断增长，直到上限
    // 3、size越大，一次向central cache要的batchNum就越小
    // 4、size越小，一次向central cache要的batchNum就越大
    size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
    // 这里要强调的是batchNum仅仅是为了提高效率，实际分配的对象数量可能不足batchNum(比如一个span里没有这么多的数量)

    // 去中心缓存获取batchNum个对象
    void *start = nullptr;
    void *end = nullptr;
    size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
    // 实际我只想要一个对象，batchNum仅仅是为了提高效率，比如batchNum为10个，可是Span里没有10个空闲的空间
    // 只有4个了，那也没有关系，通过返回值就可以知道实际返回了多少个空间

    assert(actualNum > 0);

    if (_freeLists[index].MaxSize() == batchNum)
    {
        // 即当_freeLists[i].MaxSize()<=SizeClass::NumMoveSize(size)时
        // SizeClass::NumMoveSize(size)区间为[2,512]，如果_freeLists[i].MaxSize()大于这个区间那么
        //_freeLists[i].MaxSize()就失去了作用，因此当_freeLists[i].MaxSize() != batchNum时
        // 就不用再关心_freeLists[i].MaxSize()了
        _freeLists[index].MaxSize() += 1;
    }

    if(actualNum == 1){
        assert(start == end);
        return start;

    }else{
        //  >1，返回一个，剩下挂到自由链表
        _freeLists[index].PushRange(NextObj(start) ,end);
        return start;
    }
}