#ifndef __NGX_SOCKET_EPOLL_H__
#define __NGX_SOCKET_EPOLL_H__

#include <vector>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 这里放一些宏定义
#define NGX_LISTEN_BACKLOG 1024
#define NGX_MAX_EVENTS 1024


typedef struct ngx_listen_s ngx_listen_t, *lpngx_listen_t;
typedef struct ngx_connect_s ngx_connect_t, *lpngx_connect_t;
class CSocket;

// using ngx_event_handler_ptr = void (*)(lpngx_connect_t c); // 定义成员函数指针
typedef void (CSocket::*ngx_event_handler_ptr)(lpngx_connect_t c); 

// 与监听端口有关的结构，每个监听套接字都为其分配一个ngx_connect_s结构的连接对象
struct ngx_listen_s
{
    int port;
    int fd;
    lpngx_connect_t connect;
};

// 表示一个连接的TCP结构体，作为epoll中data.ptr的传入数据结构
struct ngx_connect_s
{
    int fd;                   // 套接字句柄
    lpngx_listen_t listening; // 对应的监听套接字

    struct sockaddr_in c_sockaddr; // 对端的地址信息

    ngx_event_handler_ptr rhandle; // 对应读事件处理函数指针
    ngx_event_handler_ptr whandle; // 对应写事件处理函数指针

    lpngx_connect_t data; // 指向下一个连接对象，构成单向链表
};

// SOCKET相关类，全局只创建一个
class CSocket
{
public:
    CSocket();  //构造函数
    ~CSocket(); //释放函数
public:
    bool Initialize(); //初始化函数

public:
    int ngx_epoll_init(); //epoll功能初始化
    int ngx_epoll_add(int fd, __uint32_t events, lpngx_connect_t c);    //epoll增加事件
    int ngx_epoll_mod(int fd, __uint32_t events, lpngx_connect_t c);    //epoll修改事件
    int ngx_epoll_del(int fd, __uint32_t events, lpngx_connect_t c);    //epoll删除
    int ngx_epoll_wait(int timer); //epoll等待接收和处理事件

    void ngx_free_close_connection(lpngx_connect_t c);

private:
    int socket_bind_listen(int port);
    void ngx_close_listening_sockets(); //关闭监听套接字
    int setSocketNonBlocking(int fd);    //设置非阻塞套接字

    //一些业务处理函数handler
    void ngx_event_accept(lpngx_connect_t oldc);      //建立新连接
    void ngx_wait_request_handler(lpngx_connect_t c); //设置数据来时的读处理函数

    //连接池 或 连接 相关
    lpngx_connect_t ngx_get_connection(int isock); //从连接池中获取一个空闲连接
    void ngx_free_connection(lpngx_connect_t c);   //归还参数c所代表的连接到到连接池中

private:
    int m_worker_connections; //epoll连接的最大项数
    int m_ListenPortCount;    //所监听的端口数量
    int m_epollhandle;        //epoll_create返回的句柄

    //和连接池有关的
    lpngx_connect_t m_pconnections;      //注意这里可是个指针，其实这是个连接池的首地址
    lpngx_connect_t m_pfree_connections; //空闲连接链表头，连接池中总是有某些连接被占用，为了快速在池中找到一个空闲的连接，我把空闲的连接专门用该成员记录;
                                         //【串成一串，其实这里指向的都是m_pconnections连接池里的没有被使用的成员】
    //lpngx_event_t                  m_pread_events;                     //指针，读事件数组
    //lpngx_event_t                  m_pwrite_events;                    //指针，写事件数组
    int m_connection_n;      //当前进程中所有连接对象的总数【连接池大小】
    int m_free_connection_n; //连接池中可用连接总数

    std::vector<lpngx_listen_t> m_ListenSocketList; //监听套接字队列

    struct epoll_event m_events[NGX_MAX_EVENTS]; //用于在epoll_wait()中承载返回的所发生的事
};

#endif
