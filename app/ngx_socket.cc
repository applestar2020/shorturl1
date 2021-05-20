#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "ngx_socket.h"

int socket_bind_listen(int port)
{
    //检查端口是否在正确的区间
    if (port < 0 || port > 65535)
        port = 8080;

    // 创建TCP套接字
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
        return - 1;

    // 解决 Address already in use 错误
    int one = 1;
    if(setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one)) == -1)
        return -1;

    // 同样的方式可以设置nagle算法

    // 绑定IP和端口
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned int)port);
    
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // 开始监听
    if(listen(listen_fd,1024)==-1)
        return -1;
    
    return listen_fd;

}

int setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

