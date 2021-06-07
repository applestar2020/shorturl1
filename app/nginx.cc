// 程序入口函数

#include <iostream>
#include <unistd.h>
#include <string>

#include "ngx_gloabl.h"
#include "ngx_log.h"
#include "ngx_cfg.h"
#include "ngx_demon.h"
#include "ngx_timer.h"
#include "ngx_threadpool.h"
#include "ngx_socket_epoll.h"

using namespace std;

// 一些全局变量
extern ngx_log_t ngx_log;

//socket相关
CSocket m_socket;      //socket全局对象
Threadpool threadpool; //线程池全局对象

TimerManager timermanager;

int listen_fd = -1;

string PATH = "www";

void test(int)
{
    ngx_log_error_core(NGX_LOG_WARN, 0, "当前执行线程ID：%d ", pthread_self());
}

int main()
{
    cout << "--------------begin-----------" << endl;

    // 读取配置文件
    CConfig *p_config = CConfig::GetInstance();
    p_config->getCFG();
    // p_config->printCFG();

    // 日志文件初始化
    ngx_log_init(stoi(p_config->get_item("append")));
    ngx_log.log_level = stoi(p_config->get_item("LogLevel")); //日志等级
    ngx_log_error_core(6, 0, "--------------sever start--------------");

    // 读取网页目录，要对特殊情况进行处理
    PATH = p_config->get_item("PATH");

    // 是否以守护进程的都是运行
    if (stoi(p_config->get_item("daemon")) == 1)
    {
        int ret = ngx_demon();
        if (ret == -1)
        {
            //非正常退出
            cout << "error:" << __LINE__ << endl;
            return -1;
        }
        else if (ret == 1)
        {
            //父进程
            //是否要在这里关闭日志文件的描述符？最好手动关闭
            close(ngx_log.fd);
            return 1;
        }
    }
    
    // 主进程
    pid_t pid = getpid();
    pid_t ppid = getppid();
    ngx_log_error_core(6, 0, "pid=%d", pid);
    ngx_log_error_core(6, 0, "ppid=%d", ppid);

    // 创建线程池
    int threadnum = stoi(p_config->get_item("threadnum"));
    threadpool.addThread(threadnum);
    ngx_log_error_core(NGX_LOG_INFO, 0, "线程数量：%d ；当前空闲线程数量：%d ", threadpool.thrCount(), threadpool.idlCount());

    // socket初始化
    if (m_socket.Initialize() == false) //初始化socket
    {
        ngx_log_error_core(NGX_LOG_INFO, 0, "socket初始化有误！！");
        return 0;
    }

    m_socket.ngx_epoll_init(); //初始化epoll相关内容，同时 往监听socket上增加监听事件，从而开始让监听端口履行其职责
    ngx_log_error_core(NGX_LOG_INFO, 0, "ngx_epoll_init()初始化成功");

    int t = 1;
    while (1)
    {
        // ngx_log_error_core(6, 0, "滴答滴：%d", t++);
        // sleep(1);
        // if (t++ > 15)
        //     break;

        m_socket.ngx_epoll_wait(-1);
        // threadpool.commit(test,12);

        // timermanager.handleExpireEvent();
    }

    // string cmd = "netstat -anp | grep 443";
    // system(cmd.c_str());

    close(ngx_log.fd);

    return 0;
}
