#include <iostream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int ngx_demon()
{
    // 子进程脱离终端，
    if (setsid() == -1)
    {
        // 可以加日志输出
        return -1;
    }

    // 文件权限给最大
    umask(0);

    close(STDIN_FILENO); //关闭和终端的联系，文件描述符
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

int main()
{
    int i=0;
    pid_t pid=fork();
    if(pid<0)    
    {
        perror("fork() error:");
        return -1;
    }
    else if(pid>0)
    {
        cout<<"子进程pid:"<<pid<<endl;
        cout<<"bye"<<endl;
        return 0;
    }
    else
    {
        // 子进程
        // 子进程脱离终端，
        if (setsid() == -1)
        {
            // 可以加日志输出
            return -1;
        }

        // 文件权限给最大
        umask(0);

        close(STDIN_FILENO); //关闭和终端的联系，文件描述符
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
    while(i++<120)
    {
        sleep(1);
    }
    return 0;
    
}