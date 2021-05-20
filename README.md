[TOC]



### TODO

1. 参照nginx的目录结构等进行程序的设计，
2. 后期可以有一个改进的计划就是将每个模块都单独的拿出来放在一个文件夹中，方便后期的移植，亦可参照nginx的结构（可选）
3. **有没有其它的处理超时请求的方法**
4. 对`log`文件的使用添加设置，在配置文件中



---

### 常用命令

```c++
valgrind --tool=memcheck --leak-check=yes ./nginx
ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|nginx'
valgrind --tool=memcheck --leak-check=full --show-reachable=yes ./nginx
```



---

### 第一天 开发初始化

> 2021年5月6日 19:59:48

1. 为了解决不同机器上代码不同步的问题，将之前的腾讯云服务器系统改成了Ubuntu，同时使用[VScode远程进行开发和调试](https://www.cnblogs.com/fundebug/p/vscode-remote-development.html)



#### 小技巧

- <kbd>Alt</kbd>+<kbd>Shift</kbd>+<kbd>F</kbd>：VS代码格式化
- Linux服务相关：**`service nginx stop/start/restart/status`**
- linux目录中：
  - **/var**：是variable（变量）的缩写，这个目录下面放着不断扩充的东西，我们习惯将经常被修改的目录放到这个目录下。包括各种日志文件以及 `nginx`默认的 `www`目录
  - **/usr**：是`unix shared resources`(共享资源)的缩写，用户的很多应用程序和文件都放在这个目录下
- `misc`杂项的意思

---

### 第二天 

> 2021年5月7日 23:37:05

1. 项目中需要包含的功能：
   - makefile文件的使用，文件之间建立层次结构
   - 日志文件读取，改写成C++的形式
   - log日志的添加，改写成重定向的形式
   - socket的监听程序以及守护进程的建立
   - 使用epoll技术
   - HTML文件的解析与使用（这部分可以尽可能的熟悉，后面要使用到相关的知识）
   - 加入线程池
   - **现在才开始真正的编写短链服务器的相关代码！！**

---

### 第三天

> 2021年5月8日 08:48:26

1. Makefile文件的结构![image-20210508091308509](https://i.loli.net/2021/05/08/QU2IYNKjLHX9ypg.png)
2. **日志文件**这一块，暂时使用的原版的程序；如果要改动的话，要改的地方特别多。。先往下继续执行吧
3. 配置文件读取这一块：
   - 使用单例模式，初始化时读取配置文件并将其中的表项保存在 `unordered_map`当中以提高后续提取的速度。
   - 后期如果使用`singal`可以实现对配置文件的重新读取
4. 在**单例模式**这里遇到的一个问题是，在单例类中不释放new出来的内存，`valgrind`也并没有报错？！！！！
5. 在获取配置项时，只需要包含`ngx_cfg.h`头文件，然后获取一个实例对象，就可以从实例的map中取到数据

#### 守护进程

**[参考](https://www.ruanyifeng.com/blog/2016/02/linux-daemon.html)**

1. 后台任务的两个特点：

   - 继承当前session的标准输出（stdout）和标准错误（stderr），所以后台任务的输出也会同步地在命令行下显示

   - 不再继承当前session的标准输入（stdin），如果它试图读取标准输入，就会暂停执行

2. “后台任务”与“前台任务”的区别就是：是否继承了标准输入
3. **SIGHUP**信号：当用户准备退出session时，系统向该`session`发出 `SIGHUP`信号，session将SIGHUP信号发给子进程，子进程收到信号之后，自动退出。这是前台任务随session退出的原因；
4. 那后台任务是否也会收到 **SIGHUP**信号呢？这个跟Linux中的 `huponexit`参数有关，默认时关闭的。`shopt | grep huponexit`
5. [初始化步骤](https://xmuli.blog.csdn.net/article/details/105453850)：![image-20210508230239385](https://i.loli.net/2021/05/08/6SDhQJe27wrdiKn.png)

#### 小结

- `string` find函数会返回一个下标；如果没有，则会返回一个特别的标记 `string：：nops`

- 类中的static对象需要在外部就行初始化，否则编译的时候会报错

- 静态实例也只能操作静态成员

- 常用命令：

  ```c
  ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|nginx'
  valgrind --tool=memcheck --leak-check=full --show-reachable=yes ./nginx
  ```

---

### 第四天

> 2021年5月9日 09:09:50

1. epoll事件处理流程：![image-20210509111239228](https://i.loli.net/2021/05/09/zr41NyPDpmiaWKb.png)

2. 现在epoll的处理有两种方式：

   - epoll与requestdata结构体分开
   - 不分开，写在同一个类当中，个人认为如果不适用epoll的话

   requestdata结构体存在的意义不是很大，所以倾向于整合在一起。

#### 小结

1. `ps axj`参数含义：

   - **a**ll：表示不仅列出当前用户的进程，也列出所有其他用户的进程
   - x：表示不仅列出控制终端的进程，也列出无控制终端的进程
   - j：表示列出与作业控制相关的信息
   - -e 显示所有进程。-f 全格式。-h 不显示标题。-l 长格式。-w 宽输出。a 显示终端上的所有进程，包括其他用户的进程。r 只显示正在运行的进程。x 显示没有控制终端的进程。

2. **后期可以有一个改进的计划就是将每个模块都单独的拿出来放在一个文件夹中，方便后期的移植**

3. 在程序中执行Linux命令 `system("netstat -anp | grep 9999");`

4. `string`与`char*`[类型互转](https://blog.csdn.net/qq_18410319/article/details/90487796)：

   - ```c++
     string cmd="netstat -anp | grep "+to_string(port);
     system(cmd.c_str());
     cmd.data();// 返回之中不包括 '\0'
     ```

   - char 转 string直接复制即可

---

### 第五天

> 2021年5月10日 10:12:36

1. 现在倾向与将requestdata与sokcet的epoll部分结合在一起；

2. 监听多个端口

3. epoll与requestdata分离的做法是：

   - epoll添加监听套接字

4. `epoll_data`结构体：

   ```c++
   typedef union epoll_data {
   void *ptr;
   int fd;
   __uint32_t u32;
   __uint64_t u64;
   } epoll_data_t;
    
   struct epoll_event {
   __uint32_t events; /* Epoll events */
   epoll_data_t data; /* User data variable */
   };
   ```

   

#### 小结

1. `a=12`表达式的返回值也是12
2. `int epoll_creatre(int size);`size代表的是最大的监听数目，该值在select中，最多是1024

---

### 第六天

> 2021年5月11日 08:58:33

1. 初步完成了epoll代码的编写，ET模式，对非阻塞的理解深刻了些许

2. `accept`与`accept4`的区别？

   A：后者多了一个flag参数，可以用来设置**SOCK_NONBLOCK或者是SOCK_CLOEXEC**

3. `recv`与`read`的区别？

   A：后者同样多了一个`flag`参数，若该参数为0，则无异。

4. **如何处理过期事件？**

5. [GDB core](https://www.cnblogs.com/kuliuheng/p/11698378.html)



#### 小结

1. `ulimit -a`命令可以查看Linux下的参数限制值

   ```shell
   core file size          (blocks, -c) 0
   data seg size           (kbytes, -d) unlimited
   scheduling priority             (-e) 0
   file size               (blocks, -f) unlimited
   pending signals                 (-i) 7192
   max locked memory       (kbytes, -l) 65536
   max memory size         (kbytes, -m) unlimited
   open files                      (-n) 1024
   pipe size            (512 bytes, -p) 8
   POSIX message queues     (bytes, -q) 819200
   real-time priority              (-r) 0
   stack size              (kbytes, -s) 8192   #默认栈大小：8M
   cpu time               (seconds, -t) unlimited
   max user processes              (-u) 7192
   virtual memory          (kbytes, -v) unlimited
   file locks                      (-x) unlimited
   ```


---

### 第7天

> 2021年5月12日 09:43:07

1. 目前要解决：**处理过期事件**和 **HTTP解析**
2. [EPOLLIN , EPOLLOUT , EPOLLPRI, EPOLLERR 和 EPOLLHUP事件](https://blog.csdn.net/heluan123132/article/details/77891720)
3. [EPOLLONESHOT](https://blog.csdn.net/liuhengxiao/article/details/46911129)：相同的事件只触发一次，下次需要触发时需要重新进行设置，**尤其是在使用LT模式的时候**，同时可以解决多线程中对同一个socket产生的竞态处理
4. HTTP解析就用现成的代码吧，毕竟这个也不是重点的内容

#### 从fd中读取/写入事件

1. **websever中的写法**：

   ```c++
   ssize_t readn(int fd, void *buff, size_t n)
   {
       size_t nleft = n;
       ssize_t nread = 0;
       ssize_t readSum = 0;
       char *ptr = (char*)buff;
       while (nleft > 0)
       {
           if ((nread = read(fd, ptr, nleft)) < 0)
           {
               if (errno == EINTR)
                   nread = 0;
               else if (errno == EAGAIN)
               {
                   return readSum;
               }
               else
               {
                   return -1;
               }  
           }
           else if (nread == 0)
               break;  //EOF
           readSum += nread;
           nleft -= nread;
           ptr += nread;
       }
       return readSum;
   }
   
   ssize_t writen(int fd, void *buff, size_t n)
   {
       size_t nleft = n;
       ssize_t nwritten = 0;
       ssize_t writeSum = 0;
       char *ptr = (char*)buff;
       while (nleft > 0)
       {
           if ((nwritten = write(fd, ptr, nleft)) <= 0)
           {
               if (nwritten < 0)
               {
                   if (errno == EINTR || errno == EAGAIN)
                   {
                       nwritten = 0;
                       continue;
                   }
                   else
                       return -1;
               }
           }
           writeSum += nwritten;
           nleft -= nwritten;
           ptr += nwritten;
       }
       return writeSum;
   }
   
   ```

2. [recv()和read()](https://blog.csdn.net/hhhlizhao/article/details/73912578)

   ```c++
   ssize_t recv(int sockfd,void *buf,size_t n_byte.int flasgs);
   // 返回值：返回数据的字节长度；若无可用数据或对等方已经按序结束，返回0；若出错，返回-1.（APUE说法）
   ```

3. `write/send`出现`EAGAIN`出现的原因：

   - 套接字是异步的，所有数据提交到发送缓冲区之后是立即返回的
   - 如果发送缓冲区满了，就会提示`EAGAIN`

4. [出现EINTR的原因](https://blog.csdn.net/hnlyyk/article/details/51444617)![image-20210512111850104](https://i.loli.net/2021/05/12/xJLErhsjecOduFa.png)

5. 如果`recv`的返回值为0，说明连接已经断开![img](https://images0.cnblogs.com/blog2015/743874/201504/251610488439829.jpg)

---

### 第8天

> 2021年5月13日 08:56:19

1. 现在需要解决超时连接的问题，思路1就是优先队列：Timer与request互相指向，然后将Timer放入优先队列中，在主线程或者单独一个线程中进行遍历，对于要超时的事件进行删除。
2. 使用智能指针之后，内存自行释放，真便利
3. 不使用智能指针的话，确实会发生内存泄漏
4. 目前的测试结果，加了很多输入输出信息![image-20210513172502170](https://i.loli.net/2021/05/13/kSGb8dTzmeEarx3.png)
5. `webbench`使用IP地址和使用域名的测试结果基本一致![image-20210513195203015](https://i.loli.net/2021/05/13/M3Ye8g29hElyu7X.png)
6. mycode与nginx的对比：存在偶然的情况![image-20210513195345038](https://i.loli.net/2021/05/13/mwlsHxPGuOI5zJ1.png)
7. 发现一个细节：`netstat -anp | grep 8081`中的client端口号与发送数据时的不一致：**建立连接所用的端口和发送数据所用的端口不一样**
8. **完成线程池的移植**！

---

### 第9天

> 2021年5月14日 09:27:22

1. [线程池出处](https://www.cnblogs.com/lzpong/p/6397997.html)
2. 腾讯云服务器测试的结果![image-20210514103523714](https://i.loli.net/2021/05/14/bRdASThMX4JaBn5.png)
3. [top命令详解](https://www.cnblogs.com/mauricewei/p/10496633.html)
4. [查看磁盘性能](https://www.cnblogs.com/mauricewei/p/10502539.html)：`hdparm  -Tt /dev/sda`![image-20210514113438390](https://i.loli.net/2021/05/14/BfgxQ4vuw92khiW.png)![image-20210514113330392](https://i.loli.net/2021/05/14/DIQUAJbG8FfkvM2.png)
5. 在使用webbench进行测试的时候，发现各个进程之间的CPU时间是平均分配的，**平均分配**
6. webbench测试结果![image-20210514114947200](https://i.loli.net/2021/05/14/yqLWHmibahM4IVF.png)
7. 一般而言，**进程会随着终端的退出而退出**，除非忽略了 **SIGHUP**?信号？
8. 获取线程的ID：
   - `pthread_self（）`
   - `std::this_thread::get_id()`



---

### 阶段性小结

首先总结一下目前已经完成的工作：![image-20210514160027982](https://i.loli.net/2021/05/14/nRw1G9bf57uDC3l.png)

![image-20210514160226584](https://i.loli.net/2021/05/14/1poKblEVnkWGDma.png)

![image-20210514160611847](https://i.loli.net/2021/05/14/Su5p8FrafjNWTRx.png)

- 目前已经将所有的基本功能都实现了，后期添加短链服务的时候，预计会有几个模块添加进来
  - 请求解析与返回
  - 哈希值计算MurmurHash
  - 布隆过滤器的使用
  - 数据库

---

~~准备科研汇报两天，然后玩了两天....~~

---

### 第13天

> 2021年5月18日 09:20:36

1. ```php+HTML
   Internet Protocol Version 4, Src: 10.170.49.67, Dst: 106.52.58.96
   Transmission Control Protocol, Src Port: 8268, Dst Port: 443, Seq: 1, Ack: 1, Len: 461
   Hypertext Transfer Protocol
       [Expert Info (Warning/Security): Unencrypted HTTP protocol detected over encrypted port, could indicate a dangerous misconfiguration.]
       GET /project.html HTTP/1.1\r\n
       Host: applestar.xyz:443\r\n
       Connection: keep-alive\r\n
       Cache-Control: max-age=0\r\n
       Upgrade-Insecure-Requests: 1\r\n
       User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36\r\n
       Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3\r\n
       Accept-Encoding: gzip, deflate\r\n
       Accept-Language: zh-CN,zh;q=0.9,en;q=0.8\r\n
       \r\n
       [Full request URI: http://applestar.xyz:443/project.html]
       [HTTP request 1/1]
       [Response in frame: 238]
   ```

2. 响应报文：

   ```
   HTTP/1.1 200 OK
   Connection: keep-alive
   Keep-Alive: timeout=500
   Content-type: text/html
   Content-length: 77794
   ```

   

3. ![image-20210518100534038](https://i.loli.net/2021/05/18/FjGI2rRVpTxeq5S.png)

4. 对于一些细枝末节的东西，在那还死没有必要考虑那么清楚，现在的目标就是：**快速的实现项目中的功能**，哪怕很简陋

#### HTTP请求部分

1. HTTP请求方法：![image-20210518101849647](C:\Users\apple\AppData\Roaming\Typora\typora-user-images\image-20210518101849647.png)

2. 重定向方法：![image-20210518102232700](https://i.loli.net/2021/05/18/jFZ3ETk1UrLglAN.png)

3. HTTP请求格式示例：

   ![image-20210518160643440](https://i.loli.net/2021/05/18/dVuaGFovmCnkXsg.png)



---

### 第14天

> 2021年5月19日 10:32:25

1. [301永久重定向与302临时重定向的区别：](https://www.jianshu.com/p/995a3000f7d6?isappinstalled=0)
   - 使用302可以会发生网站劫持的问题
   - 301请求会采用浏览器中的缓存，甚至可能会改变搜索引擎的
2. 下面需要完成的几个点：
   - HTTP通信：客户端与服务器之前的信息交互
   - hash计算
   - 自定义功能的实现需要在添加数据库之后实现
3. 浏览器与服务器的交互使用`ajax`，采用POST请求，[AJAX请求示例如下](https://www.runoob.com/ajax/ajax-examples.html)

---

### 第15天

> 2021年5月20日 10:20:48

1. 初步实现了功能

2. 修改了一个从手机QQ端访问时，由于 `User-Agent`长度过长而导致的头部解析失败的一个bug

3. **HTTP通信**采用的是`AJAX`中的POST请求：

   - 最主要的部分在 `url.html`中体现，发送给服务器的数据：![image-20210520163705313](https://i.loli.net/2021/05/20/49omP1h6MkqfNYB.png)

     ![image-20210520163740846](https://i.loli.net/2021/05/20/BHdF5hYSJbmDjl6.png)

   - 服务器端只需要根据到来的POST请求将`url`与`defurl`(自定义后缀)分离出来即可

4. 效果：

   ![image-20210520164047534](https://i.loli.net/2021/05/20/QBh8d1qsyRvzKEr.png)![image-20210520164138688](https://i.loli.net/2021/05/20/hOxIFtgCXUKq3Si.png)

---

#### 小结

至此，本项目的基本功能以及完成，只是还有一些功能有待补充才能称之为第一版（**进阶**）：

- 加入hash计算
- 使用数据库进行存储（目前使用的只是map）
- 使用布隆过滤器

短链服务器只是在原来的web服务之上添加了对一些特殊的`url`进行**30x**处理，功能并不复杂，先作为一般上传到github上吧

#### 参考

- [TOPURL](http://a.topurl.cn/#/)
- [短链系统设计](https://www.cnblogs.com/rjzheng/p/11827426.html)























