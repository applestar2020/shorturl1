#include <iostream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <mutex>

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

// 多进程
void test1()
{
    int fd[2];  // fd[0]-->read  fd[1]-->write
    if(pipe(fd)==-1)
    {
        cout<<"pipe error!!"<<endl;
        return ;
    }
    pid_t pid=fork();
    if(pid==-1)
        cout<<"fork error"<<endl;
    else if(pid==0) // 子进程
    {
        close(fd[0]);
        //char s[]="hello world!";
        string s;
        cin>>s;
        const char *s1=s.c_str();

        write(fd[1],s1,strlen(s1));
        close(fd[1]);
    }
    else //父进程
    {
        close(fd[1]);
        char buf[20];
        read(fd[0],buf,sizeof(buf));
        cout<<buf<<endl;
        close(fd[0]);
    }
}

// 多线程
int total_tickets=20;
void sell_tickets(int i,mutex& mt)
{
    while(1)
    {
        usleep(1000);
        mt.lock();
        if(total_tickets>0)
            cout<<i<<":"<<pthread_self()<<":"<<total_tickets--<<endl;
        else
        {
            mt.unlock();
            return ;
        }
        
        mt.unlock();
    }
    
}
void hello_thread()
{
    cout<<"hello_thread"<<endl;
}
void test2()
{
    std::mutex mt;
    // vector<thread> vt(4);
    for(int i=0;i<4;i++)
    {
         thread t(sell_tickets,i,ref(mt));
         t.detach();
    }
    sleep(1);
}


// exit()  与 _exit() 的区别
void test3()
{
    printf("hello");
    _exit(0);
}

// wait() 等待子进程正常退出
void test4()
{
    pid_t pid=fork();
    if(pid==0)
    {
        
    }
}
int main()
{
    test3();

    return 12;
}