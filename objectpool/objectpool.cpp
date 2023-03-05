#include <iostream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
// 直接去堆上按页申请空间
inline static void *SystemAlloc(size_t kpage)
{
#ifdef _WIN32
    void *ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    void *ptr = sbrk(kpage << 13);
#endif

    if (ptr == nullptr)
        throw std::bad_alloc();

    return ptr;
}

// 定长内存池
template <class T>
class ObjectPool
{
private:
    char *_memory = nullptr;   // 指向大块内存的指针
    size_t _remainBytes = 0;   // 大块内存在切分过程中剩余字节数
    void *_freelist = nullptr; // 还回来过程中链接的自由链表的头指针/自由链表的头指针，用于保存当前有哪些对象可以被重复利用。

public:
    T *New()
    {
        T *obj = nullptr;
        // 优先把还回来内存块对象，再次重复利用
        // 如果自由链表非空，以“头删”的方式从自由链表取走内存块，重复利用
        if (_freelist)
        {
            void *next = *((void **)_freelist);
            obj = (T *)_freelist;
            _freelist = next;
        }
        else
        {
            // 剩余内存不够一个对象大小时，则重新开一大块空间
            if (_remainBytes < sizeof(T))
            {
                _remainBytes = 128 * 1024;
                // >>13 其实就是一页8KB的大小，可以得到具体多少页
                _memory = (char *)SystemAlloc(_remainBytes >> 13); // 一页8kb
                if (_memory == nullptr)
                {
                    throw std::bad_alloc();
                }
            }
            obj = (T *)_memory;
            size_t objSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
            _memory += objSize;
            _remainBytes -= objSize;
        }
        // 定位new，显示调用T的构造函数初始化
        new (obj) T;
        return obj;
    }

    // 将obj这块内存链接到_freeList中
    void Delete(T *obj)
    {
        // 显示调用析构函数清理对象
        obj->~T();

        // 头插
        *(void **)obj = _freelist; // 这就必须要保证一个obj大小是>=sizeof(void*)
        _freelist = obj;
    }
};

struct TreeNode
{
    int _val;
    TreeNode *_left;
    TreeNode *_right;

    TreeNode() : _val(0), _left(nullptr), _right(nullptr) {}
    TreeNode(int x) : _val(x), _left(nullptr), _right(nullptr) {}
};

void TestObjectPool()
{
    // 申请释放的轮次
    const size_t Rounds = 5;
    // 每轮申请释放多少次
    const size_t N = 100000;

    std::vector<TreeNode *> v1;
    v1.reserve(N);

    size_t begin1 = clock();
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (size_t i = 0; i < N; ++i)
        {
            v1.push_back(new TreeNode);
        }
        for (size_t i = 0; i < N; ++i)
        {
            delete v1[i];
        }
        v1.clear();
    }
    size_t end1 = clock();

    std::vector<TreeNode *> v2;
    v2.reserve(N);

    ObjectPool<TreeNode> TNPool;
    size_t begin2 = clock();
    for (size_t j = 0; j < Rounds; ++j)
    {
        for (size_t i = 0; i < N; ++i)
        {
            v2.push_back(TNPool.New());
        }
        for (int i = 0; i < N; ++i)
        {
            TNPool.Delete(v2[i]);
        }
        v2.clear();
    }
    size_t end2 = clock();

    std::cout << "new cost time:" << end1 - begin1 << std::endl;
    std::cout << "object pool time:" << end2 - begin2 << std::endl;
}

int main(int argc, char const *argv[])
{
    TestObjectPool();

    return 0;
}