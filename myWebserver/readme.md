The Avengers
author: xiongaodi

为《复仇者联盟》中，你喜欢的角色投票。

多线程web服务器，可以解析http请求。
1.使用线程池+非阻塞socket+epoll+半同步半反应堆模式的并发模型
2.使用string类相关函数解析HTTP请求报文，支持解析GET和POST请求
3.利用共享内存和IO向量机制实现页面返回。
4.使用redis保存用户的数据

线程安全类：
sem
mutex

线程池类：
维护线程列表和请求列表
主线程用于监听套接字，读取（创建http对象并且入队）和写入响应报文
工作线程用于接受连接，处理请求
请求列表中为http请求对象，各个线程竞争请求列表中的锁

http请求类：
维护连接，用读写套接字来初始化
实现报文解析，支持GET和POST
通过共享内存获得指向请求文件资源的指针，通过io向量机制，将响应行和内容聚集写入，返回响应报文

main函数：
创建线程池
根据最大描述符来创建http请求数组，用来根据不同请求的不同描述符来记录各个http请求
socket,bind,listen,epoll_wait,得到io事件event
对于新的连接请求，接受，保存入http请求数组进行初始化
对于读写请求，请求入队

数据库
使用redis键值对来储存用户名和密码
sorted-set 记录投票
单例模式创建redis客户端类，用于用户信息查询,包装了常用的查询命令，并对结果进行解析
函数getReply(向redis服务端发送请求)进行了加锁保护，防止同一块区域被同时改写而导致返回结果异常
需要拥有redis环境
安装redis sudo apt-get install redis-server
安装C++的hiredis库 sudo apt-get install libhiredis-dev

定时器
使用定时器清理非活跃连接
创建管道，信号（定时触发或按键触发）被触发时，向管道内写入
epoll监听管道一端的读事件
根据读出的信号不同，进行定时清理或关闭服务的操作
信号handler仅向管道中写数据，用于更新标志位，并不进行真正的清理操作，保证处理足够快速
主函数循环中根据标志位的变化来执行具体的操作

致谢：
JohnSnow，TinyWebServer。
Linux高性能服务器编程，游双著.