#include <iostream>
#include <thread>
#include <time.h>
#include <mutex>
#include <atomic>

using namespace std;

class A
{
public:
    A()
    {
        cout << "constract " << endl;
    }
    ~A()
    {
        cout << "~unconstract " << endl;
        delete this;
    }
};

void test1()
{
    A *a = new A();
    // delete a;
    // a->~A();
    // A a=A();
}

int main()
{
    test1();

    return 0;
}