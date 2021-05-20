
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ngx_demon.h"

int ngx_demon()
{
    switch (fork())
    {
    case -1: // 创建子进程失败
        return -1;
    case 0: // 子进程，跳出往下执行
        break;
    default: // 父进程，不知道之前的日志文件描述符之类的是否需要关闭？
        return 1;
    }

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
