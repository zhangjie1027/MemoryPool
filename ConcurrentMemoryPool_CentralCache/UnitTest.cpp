#include "ObjectPool.hpp"
#include "ConcurrentAlloc.hpp"

void Alloc1()
{
    for (size_t i = 0; i < 5;++i){
        void *ptr = ConcurrentAlloc(6);
    }
}

void Alloc2(){
    for(size_t i = 0;i< 5;++i){
        void *ptr = ConcurrentAlloc(7);
    }
}

void TLSTest(){
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);
    t1.join();
    t2.join();
}

int main(int argc, char const *argv[])
{
    TLSTest();

    return 0;
}
