

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

#include "ngx_cfg.h"
#include "ngx_log.h"
#include "ngx_request.h"
#include "ngx_threadpool.h"
#include "ngx_socket_epoll.h"



extern string PATH;
extern Threadpool threadpool;
extern TimerManager timermanager;


// 构造函数
CSocket::CSocket() : m_worker_connections(1), m_ListenPortCount(1), m_epollhandle(-1),
                     m_pconnections(nullptr), m_pfree_connections(nullptr), m_connection_n(0), m_free_connection_n(0)
{
}

// 析构函数
CSocket::~CSocket()
{
    for (auto iter = m_ListenSocketList.begin(); iter != m_ListenSocketList.end(); iter++)
    {
        delete (*iter);
    }
    m_ListenSocketList.clear();

    if (m_pconnections)
        delete[] m_pconnections;
}

// 初始化函数
bool CSocket::Initialize()
{
    // 读取配置参数，这里stoi的异常后面要改变cfg的函数来修正
    CConfig *p_config = CConfig::GetInstance();
    m_worker_connections = stoi(p_config->get_item("worker_connections")); //epoll连接的最大项数
    m_ListenPortCount = stoi(p_config->get_item("ListenPortCount"));       //取得要监听的端口数量

    for (int i = 0; i < m_ListenPortCount; i++)
    {
        string sport = "listenport" + to_string(i + 1);
        int port = stoi(p_config->get_item(sport));
        int listen_fd = CSocket::socket_bind_listen(port);

        setSocketNonBlocking(listen_fd);

        if (listen_fd < 0)
        {
            ngx_log_stderr(errno, "CSocekt::Initialize()中监听%d端口失败", port);
            return false;
        }

        lpngx_listen_t p_listensocketitem = new ngx_listen_t;         //千万不要写错，注意前边类型是指针，后边类型是一个结构体
        memset(p_listensocketitem, 0, sizeof(ngx_listen_t));          //注意后边用的是 ngx_listening_t而不是lpngx_listening_t
        p_listensocketitem->port = port;                              //记录下所监听的端口号
        p_listensocketitem->fd = listen_fd;                           //套接字木柄保存下来
        ngx_log_error_core(NGX_LOG_INFO, 0, "监听%d端口成功!", port); //显示一些信息到日志中
        m_ListenSocketList.push_back(p_listensocketitem);             //加入到队列中
    }

    if (m_ListenSocketList.empty())
        return false;

    return true;
}

//--------------------------------------------------------------------
//socket相关
int CSocket::socket_bind_listen(int port)
{
    //检查端口是否在正确的区间
    if (port < 0 || port > 65535)
        port = 8080;

    // 创建TCP套接字
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
        return -1;

    // 解决 Address already in use 错误
    int one = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)
        return -1;

    // 同样的方式可以设置nagle算法

    // 绑定IP和端口
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned int)port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // 开始监听
    if (listen(listen_fd, 1024) == -1)
        return -1;

    return listen_fd;
}

int CSocket::setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

//--------------------------------------------------------------------
//epoll相关
int CSocket::ngx_epoll_init()
{
    m_epollhandle = epoll_create(m_worker_connections);
    if (m_epollhandle < 0)
    {
        ngx_log_stderr(errno, "CSocekt::ngx_epoll_init()中epoll_create()失败.");
        return -1;
    }

    // 创建连接池数组，后续用于处理客户端的连接
    m_connection_n = m_worker_connections; // 连接池连接总数
    // 连接池数组
    m_pconnections = new ngx_connect_t[m_connection_n];

    //构建连接单向链表
    lpngx_connect_t next = nullptr;
    lpngx_connect_t c = m_pconnections; // 对应数组首地址

    for (int i = m_connection_n - 1; i >= 0; i--)
    {
        c[i].data = next;
        c[i].fd = -1;
        next = &c[i];
    }

    m_pfree_connections = next;           // 空闲连接目前也指向首地址，
    m_free_connection_n = m_connection_n; // 目前都是空闲连接

    // 遍历所有【监听端口】，并给每个监听端口分配一个连接池中的连接，相当于给它一块内存空间用于存储信息
    for (auto iter = m_ListenSocketList.begin(); iter != m_ListenSocketList.end(); iter++)
    {
        c = ngx_get_connection((*iter)->fd);

        // 连接对象和监听对象相互关联
        c->listening = (*iter);
        (*iter)->connect = c;

        c->rhandle = &CSocket::ngx_event_accept;

        // 往epoll中添加监听事件
        __uint32_t event = EPOLLIN | EPOLLET;
        ngx_epoll_add((*iter)->fd, event, c);
    }

    return 1;
}

int CSocket::ngx_epoll_add(int fd, __uint32_t events, lpngx_connect_t c)
{
    struct epoll_event event;
    event.data.ptr = c;
    event.events = events;
    if (epoll_ctl(m_epollhandle, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll add eeeor");
        return -1;
    }
    return 0;
}

int CSocket::ngx_epoll_mod(int fd, __uint32_t events, lpngx_connect_t c)
{
    struct epoll_event event;
    event.data.ptr = c;
    event.events = events;
    if (epoll_ctl(m_epollhandle, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        perror("epoll add eeeor");
        return -1;
    }
    return 0;
}

int CSocket::ngx_epoll_del(int fd, __uint32_t events, lpngx_connect_t c)
{
    struct epoll_event event;
    event.data.ptr = c;
    event.events = events;
    if (epoll_ctl(m_epollhandle, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        perror("epoll add eeeor");
        return -1;
    }
    return 0;
}


int CSocket::ngx_epoll_wait(int timer)
{
    int event_num = epoll_wait(m_epollhandle, m_events, NGX_MAX_EVENTS, timer);

    if (event_num == -1)
    {
        //有错误发生，发送某个信号给本进程就可以导致这个条件成立，而且错误码根据观察是4；
        //#define EINTR  4，EINTR错误的产生：当阻塞于某个慢系统调用的一个进程捕获某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误。
        //例如：在socket服务器端，设置了信号捕获机制，有子进程，当在父进程阻塞于慢系统调用时由父进程捕获到了一个有效信号时，内核会致使accept返回一个EINTR错误(被中断的系统调用)。
        if (errno == EINTR)
        {
            //信号所致，直接返回，一般认为这不是毛病，但还是打印下日志记录一下，因为一般也不会人为给worker进程发送消息
            ngx_log_error_core(NGX_LOG_INFO, errno, "CSocekt::ngx_epoll_process_events()中epoll_wait()失败!");
            return 1; //正常返回
        }
        else
        {
            //这被认为应该是有问题，记录日志
            ngx_log_error_core(NGX_LOG_ALERT, errno, "CSocekt::ngx_epoll_process_events()中epoll_wait()失败!");
            return 0; //非正常返回
        }
    }

    if (event_num == 0) //超时，但没事件来
    {
        if (timer != -1)
        {
            //要求epoll_wait阻塞一定的时间而不是一直阻塞，这属于阻塞到时间了，则正常返回
            return 1;
        }
        //无限等待【所以不存在超时】，但却没返回任何事件，这应该不正常有问题
        ngx_log_error_core(NGX_LOG_ALERT, 0, "CSocekt::ngx_epoll_process_events()中epoll_wait()没超时却没返回任何事件!");
        return 0; //非正常返回
    }

    // 正常收到
    // ngx_log_error_core(NGX_LOG_INFO, 0, "收到了%d个事件", event_num);
    for (int i = 0; i < event_num; i++)
    {
        lpngx_connect_t c = (lpngx_connect_t)(m_events[i].data.ptr);

        uint32_t revents = m_events[i].events; //取出事件类型

        if (revents & (EPOLLERR | EPOLLHUP)) //例如对方close掉套接字，这里会感应到【换句话说：如果发生了错误或者客户端断连】
        {
            struct in_addr in = (c->c_sockaddr).sin_addr;
            char str[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN这个宏系统默认定义 16
            //成功的话此时IP地址保存在str字符串中。
            inet_ntop(AF_INET, &in, str, sizeof(str));
            ngx_log_error_core(NGX_LOG_INFO, 0, "客户端%s:%d 已关闭", str, (c->c_sockaddr).sin_port);
            //这加上读写标记，方便后续代码处理，至于怎么处理，后续再说，这里也是参照nginx官方代码引入的这段代码；
            // revents |= EPOLLIN | EPOLLOUT; //EPOLLIN：表示对应的链接上有数据可以读出（TCP链接的远端主动关闭连接，也相当于可读事件，因为本服务器小处理发送来的FIN包）
            //EPOLLOUT：表示对应的连接上可以写入数据发送【写准备好】
        }

        if (revents & EPOLLIN)
        {
            // ngx_log_error_core(NGX_LOG_INFO, 0, "EPOLL收到数据了！！");
            // (this->*(c->rhandle))(c);
            (this->*(c->rhandle))(c); //注意括号的运用来正确设置优先级，防止编译出错；【如果是个新客户连入
                                      //如果新连接进入，这里执行的应该是CSocekt::ngx_event_accept(c)】
                                      //如果是已经连入，发送数据到这里，则这里执行的应该是 CSocekt::ngx_wait_request_handler
        }

        if (revents & EPOLLOUT) //如果是写事件
        {
            //....待扩展

            ngx_log_stderr(errno, "111111111111111111111111111111.");
        }
    }

    return 1;
}

//--------------------------------------------------------------------
//连接（池）相关
// 从空闲连接从获取一个连接
lpngx_connect_t CSocket::ngx_get_connection(int fd)
{
    lpngx_connect_t c = m_pfree_connections; //获取空闲链表头

    m_pfree_connections = c->data; // 指向下一个
    m_free_connection_n--;         // 空闲连接数目减少一个

    c->fd = fd;

    return c;
}

//归还参数c所代表的连接到到连接池中，注意参数类型是lpngx_connection_t
void CSocket::ngx_free_connection(lpngx_connect_t c)
{
    c->data = m_pfree_connections; //回收的节点指向原来串起来的空闲链的链头

    m_pfree_connections = c; //修改 原来的链头使链头指向新节点
    ++m_free_connection_n;   //空闲连接多1
}

//--------------------------------------------------------------------
//业务处理相关

//接收监听套接字的新连接
void CSocket::ngx_event_accept(lpngx_connect_t oldc)
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    int accept_fd = 0;
    while ((accept_fd = accept4(oldc->fd, (struct sockaddr *)&client_addr, &client_addr_len, SOCK_NONBLOCK)) > 0)
    // while ((accept_fd = accept(oldc->fd, (struct sockaddr *)&client_addr, &client_addr_len)) > 0)
    {
        struct in_addr in = client_addr.sin_addr;
        char str[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN这个宏系统默认定义 16
        //成功的话此时IP地址保存在str字符串中。
        inet_ntop(AF_INET, &in, str, sizeof(str));
        ngx_log_error_core(NGX_LOG_INFO, 0, "监听套接字：%d\tclient:%s:%d ", oldc->fd, str, client_addr.sin_port);

        lpngx_connect_t newc = ngx_get_connection(accept_fd);

        memcpy(&newc->c_sockaddr, &client_addr, client_addr_len);
        // // 设为非阻塞模式,使用了accept4
        // int ret = setSocketNonBlocking(accept_fd);
        // if (ret < 0)
        // {
        //     perror("Set non block failed!");
        //     break;
        // }

        newc->listening = oldc->listening;

        newc->rhandle = &CSocket::ngx_wait_request_handler;
        // 文件描述符可以读，边缘触发(Edge Triggered)模式，保证一个socket连接在任一时刻只被一个线程处理
        __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
        // __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
        ngx_epoll_add(accept_fd, _epo_event, newc);

        ngx_log_error_core(NGX_LOG_INFO, 0, "accept_fd:%d 加入了epoll中", accept_fd);
        // break;
    }
}

void requesthandle(lpngx_connect_t c)
{
    ngx_log_error_core(NGX_LOG_WARN, 0, "当前执行线程ID：%d ", pthread_self());
    shared_ptr<requestData> req(new requestData(c,PATH));
    req->handleRequest();
}

void CSocket::ngx_wait_request_handler(lpngx_connect_t c)
{
    threadpool.commit(requesthandle,c);
}

// 处理完成之后释放连接并关闭套接字
void CSocket::ngx_free_close_connection(lpngx_connect_t c)
{
    int fd=c->fd;
    ngx_free_connection(c);
    c->fd=-1;
    close(fd);
    ngx_log_error_core(NGX_LOG_INFO, 0, "fd:%d 已关闭", fd);
}

// void CSocket::ngx_wait_request_handler(lpngx_connect_t c)
// {
//     //ET测试代码
//     unsigned char buf[MAX_BUFF] = {0};
//     memset(buf, 0, sizeof(buf));
//     do
//     {
//         int n = readn(c->fd, buf, MAX_BUFF); //每次只收两个字节
//         if (n == -1 && errno == EAGAIN)
//             break;        //数据收完了
//         else if (n == -2) // 对方关闭连接
//         {
//             ngx_free_connection(c);
//             close(c->fd);
//             ngx_log_error_core(NGX_LOG_INFO, 0, "对方关闭连接，fd: %d 已经释放", c->fd);
//             c->fd = -1;
//             break;
//         }
//         else if (n == 0)
//             break;

//         struct in_addr in = (c->c_sockaddr).sin_addr;
//         char str[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN这个宏系统默认定义 16
//         //成功的话此时IP地址保存在str字符串中。
//         inet_ntop(AF_INET, &in, str, sizeof(str));
//         ngx_log_error_core(NGX_LOG_INFO, 0, "从%s:%d 收到%d个字节,内容为%s", str, (c->c_sockaddr).sin_port, n, buf);
//         writen(c->fd, buf, sizeof(buf));
//     } while (1);
// }


