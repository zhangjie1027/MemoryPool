#include <iostream>
#include <thread>

using namespace std;
__thread int iVar = 100; // 每个线程独有的一个全局变量

void Thread1(){
    iVar += 100;
    cout <<"Thread1 Val: " << iVar << endl;
}

void Thread2(){
    iVar += 400;
    cout <<"Thread2 Val: " << iVar << endl;
}

int main(int argc, char const *argv[])
{
    thread t1(Thread1);
    thread t2(Thread2);

    t1.join();
    t2.join();
    
    return 0;
}
