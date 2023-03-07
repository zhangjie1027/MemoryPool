#pragma once
#include "Common.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


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
            obj = (T *)_freelist;
            _freelist = NextObj(_freelist);
        }
        else
        {
            size_t objSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
            // 剩余内存不够一个对象大小时，则重新开一大块空间
            if (_remainBytes < sizeof(T))
            {
                _remainBytes = 128 * 1024;
                // >>13 其实就是一页8KB的大小，可以得到具体多少页
                _memory = (char *)SystemAlloc(_remainBytes >> 13); // 一页8kb
                // if (_memory == nullptr)
                // {
                //     throw std::bad_alloc();
                // }
            }
            obj = (T *)_memory;
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
        NextObj(obj) = _freelist; // 这就必须要保证一个obj大小是>=sizeof(void*)
        _freelist = obj;
    }
};

