#include "ConcurrentAlloc.hpp"
#include <ctime>
using std::cout;
using std::endl;

// 基准测试
// ntimes 一轮申请和释放内存的次数
// rounds 轮次
// nworks 线程数
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    clock_t malloc_costtime = 0;
    clock_t free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&, k]()
                                 {
            std::vector<void*>v;
            v.reserve(ntimes);
            for(size_t j =0; j < rounds;++j){
                clock_t begin1 = clock();
                for (size_t i = 0; i < ntimes; ++i){
                    v.push_back(malloc(16));
                    // v.push_back(malloc((16+i)%8192+1));
                }
                clock_t end1 = clock();
                clock_t begin2 = clock();
                for (size_t i =0 ; i<ntimes;++i){
                    free(v[i]);
                }
                clock_t end2  =clock();
                v.clear();
                malloc_costtime += (end1-begin1);
                free_costtime += (end2-begin2);
            } });
    }
    for (auto &t : vthread)
    {
        t.join();
    }
    cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次malloc" << ntimes << "次: 花费：" << malloc_costtime << "ms" << endl;
    cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次free" << ntimes << "次: 花费：" << free_costtime << "ms" << endl;
    cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次malloc&free" << ntimes << "次: 花费：" << (malloc_costtime + free_costtime) << "ms" << endl;
}
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    clock_t malloc_costtime = 0;
    clock_t free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&]()
                                 {
            std::vector<void*>v;
            v.reserve(ntimes);
            for(size_t j =0; j < rounds;++j){
                clock_t begin1 = clock();
                for (size_t i = 0; i < ntimes; ++i){
                    v.push_back(ConcurrentAlloc(16));
                    // v.push_back(malloc((16+i)%8192+1));
                }
                clock_t end1 = clock();
                clock_t begin2 = clock();
                for (size_t i =0 ; i<ntimes;++i){
                    ConcurrentFree(v[i]);
                }
                clock_t end2  =clock();
                v.clear();
                malloc_costtime += (end1-begin1);
                free_costtime += (end2-begin2);
            } });
    }
    for (auto &t : vthread)
    {
        t.join();
    }
    cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次concurrent malloc" << ntimes << "次: 花费：" << malloc_costtime << "ms" << endl;
    cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次concurrent free" << ntimes << "次: 花费：" << free_costtime << "ms" << endl;
    cout << nworks << "个线程并发执行" << rounds << "轮次，每轮次concurrent malloc&free" << ntimes << "次: 花费：" << (malloc_costtime + free_costtime) << "ms" << endl;
}

int main(int argc, char const *argv[])
{
    size_t n = 1000;
    cout << "==========================================================" << endl;
    BenchmarkConcurrentMalloc(n, 4, 10);
    cout << endl
         << endl;

    BenchmarkMalloc(n, 4, 10);
    cout << "==========================================================" << endl;
    // system("pause");
    return 0;

    return 0;
}
