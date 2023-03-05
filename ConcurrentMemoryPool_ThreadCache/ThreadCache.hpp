#include "Common.hpp"

// 不要用STL的数据结构，因为可能我们的内存池就是用来服务STL的
// 以及malloc，因为我们的内存池将来就是要替代malloc的

//_freeLists是一个映射自由链表的哈希桶，即_freeLists[16]，只是桶里存的不是数据，而是obj内存块的地址
//_freeLists数组存放的是头结点_head,由_head来链接obj内存块
// 这样申请某个大小的内存直接找到自由链表_freeLists取内存，释放则将内存挂在_freeLists对应下标的地方
class ThreadCache
{
private:
    FreeList _freeLists[NFREELIST];

public:
    // 申请和释放内存
    // 需要通过size的大小进行映射找到对应大小的内存桶，获取一个obj内存块
    // size表示obj内存块的大小
    void *Alloccate(size_t size);

    // 回收一个内存块将其挂在_freeLists下
    void Deallocate(void *ptr, size_t size);

    // 从中心缓存获取对象
    // index表示freeLists数组的下标
    void *FetchFromCentralCache(size_t index, size_t size);
};

// 使用线程本地存储给每个线程开辟一个属于自己独有的全局变量
#ifdef _WIN32
    static __declspec(thread) ThreadCache *pTLSThreadCache = nullptr;
#else
    static __thread ThreadCache *pTLSThreadCache = nullptr;
#endif