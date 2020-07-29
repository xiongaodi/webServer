#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "../lock/lock.h"
#include "../user/redis.h"

#include<errno.h>
#include<fcntl.h>
#include<map>
#include<iostream>
#include<netinet/in.h>
#include<sys/epoll.h>
#include<sys/mman.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<unistd.h>
#include<string.h>

using namespace std;

class http_conn //http连接类
{
public:
    static const int READ_BUFF_SIZE = 2048;     //设置读缓冲区m_read_buf大小
    static const int WRITE_BUFFER_SIZE = 2048;  //设置写缓冲区m_write_buf大小
    enum METHOD //报文请求方法
    {
        GET  = 0,
        POST
    };

    enum MAIN_STATE //主状态机状态
    {
        REQUESTLINE = 0,
        HEADER,
        CONTENT
    };

    enum LINE_STATE // 从状态机状态
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    enum HTTP_CODE  //报文解析的结果
    {
        NO_REQUEST,
        GET_REQUEST,
        POST_REQUEST,
        NO_RESOURCE,
        CLOSED_CONNEXTION
    };
public:
    http_conn(){}
    ~http_conn(){}
public:
    void init(int socketfd, const sockaddr_in& addr);   //初始化套接字
    void init();                                        //实现具体各个参数值的初始化
    void close_conn(string msg = "");                   //关闭连接
    void process();                                     //处理请求
    bool read_once();                                    //一次性调用recv读取所有数据，读取浏览器发来的全部数据，读到读缓冲区,返回调用是否称成功的信息
    bool write();

public:
    static int m_epollfd;                               //当前http连接的epoll描述符,这个是静态的
    static int m_user_count;

private:
    HTTP_CODE process_read();                           //从读缓冲区读取出来数据进行解析
    bool process_write(HTTP_CODE ret);                  //写入响应到写缓冲区中

    void parser_header(const string &text, map<string, string> &m_map);      //解析请求的内容
    void parser_requestline(const string &text, map<string, string> &m_map); //解析请求的第一行
    void parser_postinfo(const string &text, map<string, string> &m_map);    //解析post请求正文

    bool do_request(); //确定到底请求的是哪一个页面

    void unmap();

private:
    locker m_redis_lock;
    int m_socket;
    sockaddr_in m_addr;

    struct stat m_file_stat;
    struct iovec m_iv[2];        //io向量机制iovec
    int m_iovec_length;

    string m_filename;
    string m_postmsg;

    char *m_file_addr;          //读取服务器上的文件地址
    char post_temp[1024];
    char m_read_buf[READ_BUFF_SIZE];
    char m_write_buff[WRITE_BUFFER_SIZE];

    int m_read_idx;                 //读取的位置
    int m_write_idx;                //buffer中的长度

    map<string, string> m_map; //http连接的各项属性
};





#endif