#include "http_conn.h"

//对文件描述符设置非阻塞
void setnonblocking(int socketfd)
{
    int oldfd = fcntl(socketfd, F_GETFL);
    int newfd = oldfd | O_NONBLOCK;
    fcntl(socketfd, F_SETFL, newfd);
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void addfd(int epollfd, int socketfd)
{
    //把该描述符添加到epoll的事件表
    epoll_event  event;
    event.data.fd = socketfd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;          //监听输入、边缘触发、挂起
    epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event);  //将该事件添加
    setnonblocking(socketfd);
}

//从内核时间表删除描述符
void removefd(int epollfd, int socketfd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, socketfd, 0);
    close(socketfd);
}

//将事件重置为EPOLLONESHOT
void modfd(int epollfd, int socketfd, int ev)
{
    epoll_event event;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    event.data.fd = socketfd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, socketfd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

//关闭连接，关闭一个连接，客户总量减一
void http_conn::close_conn(string msg)
{
    if(m_socket != -1)
    {
        removefd(m_epollfd, m_socket);
        m_user_count--;
        m_socket = -1;
    }
}

//初始化连接,外部调用初始化套接字地址
void http_conn::init(int socketfd, const sockaddr_in &addr)
{
    m_socket = socketfd;
    m_addr = addr;
    addfd(m_epollfd, m_socket);
    init();
}

//初始化新接受的连接
void http_conn::init()
{
    m_filename = "";
    memset(m_read_buf, '\0', READ_BUFF_SIZE);
    memset(m_write_buff, '\0', WRITE_BUFFER_SIZE);
    m_read_idx = 0;
    m_write_idx = 0;

}

//对请求进行处理
void http_conn::process()
{
    //首先进行报文的解析
    HTTP_CODE ret = process_read();
    if(ret == NO_REQUEST)
    {
        modfd(m_epollfd, m_socket, EPOLLIN);
        return;
    }
    //然后进行报文的响应
    bool result = process_write(ret);
    modfd(m_epollfd, m_socket, EPOLLOUT);//最后向epoll的监听的事件表中添加可写事件
}

//解析请求的第一行
void http_conn::parser_requestline(const string&text, map<string, string>&m_map)
{
    string method = text.substr(0, text.find(" "));
    string url = text.substr(text.find_first_of(" ") + 1, text.find_last_of(" ") - text.find_first_of(" ") - 1);
    string protocol = text.substr(text.find_last_of(" ") + 1);
    m_map["method"] = method;
    m_map["url"] = url;
    m_map["protocol"] = protocol;
}

//解析请求的内容
void http_conn::parser_header(const string &text, map<string, string> &m_map)
{
    if (text.size() > 0)
    {
        if (text.find(": ") <= text.size())
        {
            string m_type = text.substr(0, text.find(": "));
            string m_content = text.substr(text.find(": ") + 2);
            m_map[m_type] = m_content;
        }
        else if (text.find("=") <= text.size())
        {
            string m_type = text.substr(0, text.find("="));
            string m_content = text.substr(text.find("=") + 1);
            m_map[m_type] = m_content;
        }
    }
}

//解析post请求正文
void http_conn::parser_postinfo(const string &text, map<string, string> &m_map)
{
    //username=xiongaodi &passwd=123456 &votename=alibaba
    //cout << "post:   " << text << endl;
    string processd = "";
    string strleft = text;
    while (true)
    {
        processd = strleft.substr(0, strleft.find("&"));
        m_map[processd.substr(0, processd.find("="))] = processd.substr(processd.find("=") + 1);
        strleft = strleft.substr(strleft.find("&") + 1);
        if (strleft == processd)
            break;
    }
}

http_conn::HTTP_CODE http_conn::process_read()
{
    string head = "";
    string left = m_read_buf; //把读入缓冲区的数据转化为string
    int flag = 0;
    int do_post_flag = 0;
    while(true)
    {
        //对每一行进行处理
        head = head.substr(0, left.find("\r\n"));
        left = left.substr(left.find("\r\n")+2);
        if(flag == 0)
        {
            flag = 1;
            parser_requestline(head, m_map);
        }

        else if(do_post_flag)
        {
            parser_postinfo(head, m_map);
            break;
        }

        else
        {
            parser_header(head, m_map);
        }
        if(head == "")
            do_post_flag = 1;
        if(left == "")
            break;
    }

    if(m_map["method"] == "POST")
    {
        return POST_REQUEST;
    }

    else if(m_map["method"] == "GET")
    {
        return GET_REQUEST;
    }
    else
    {
        return NO_REQUEST;
    }
}

bool http_conn::do_request()
{
    //区分get和post都请求了那些文件或网页
    if(m_map["method"] == "POST")
    {
        redis_client * redis = redis_client::getinstance();
        if(m_map["url"] == "./base.html" || m_map["url"] == "/")    //如果来自于登录界面
        {
            //处理登录请求
            if(redis->getUserpasswd(m_map["username"]) == m_map["passwd"])
            {
                if(redis->getUserpasswd(m_map["username"]) == "root")
                    m_filename = "./root/welcomeroot.html";         //登录进入欢迎界面
                else   
                    m_filename = "./root/welcome.html";             //登录进入欢迎界面
            }
            else
            {
                m_filename = "./root/error.html"; //进入登录失败界面
            }
        }
        else if(m_map["url"] == "/regester.html")   //如果来自注册界面    
        {
            redis->setUserpasswd(m_map["username"], m_map["passwd"]);
            m_filename = "./root/regester.html";
        }

        else if (m_map["url"] == "./welcome.html") //如果来自登录后界面
        {
            redis->vote(m_map["votename"]);
            m_postmsg = "";
            return false;
        }
        else if (m_map["url"] == "/getvote")    //如果主页要请求投票
        {
            //读取redis
            m_postmsg = redis->getvoteboard();

            return false;
        }
        else
        {
            m_filename = "./root/base.html";    //进入初始登录界面
        }
    }
    else if (m_map["method"] == "GET")
    {
        if (m_map["url"] == "/")
        {
            m_map["url"] = "/base.html";
        }
        m_filename = "./root" + m_map["url"];
    }
    else
    {
        m_filename = "./root/error.html";
    }
    return true;
    
}

void http_conn::unmap()
{
    if(m_file_addr)
    {
        munmap(m_file_addr, m_file_stat.st_size);
        m_file_addr = 0;
    }
}

bool http_conn::process_write(HTTP_CODE ret)
{
    if(do_request())
    {
        int fd = open(m_filename.c_str(), O_RDONLY);

        stat(m_filename.c_str(), &m_file_stat);
        m_file_addr = (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

        m_iv[1].iov_base = m_file_addr;
        m_iv[1].iov_len = m_file_stat.st_size;
        m_iovec_length = 2;
        close(fd); //关闭描述符了
    }
    else 
    {
        if(m_postmsg != "")
        {
            if(m_postmsg.length() < 20)
                cout<<"wrong pstmsg:" <<m_postmsg<<endl;
            
            strcpy(post_temp, m_postmsg.c_str());
            m_iv[1].iov_base = post_temp;
            m_iv[1].iov_len = (m_postmsg.size()) * sizeof(char);
            m_iovec_length = 2; 
        }
        else 
        {
            m_iovec_length = 1;
        }
    }

    return true;
}

//把socket的东西全部读到读缓冲区里面
bool http_conn::read_once()
{
    if(m_read_idx > READ_BUFF_SIZE) //如果当前可以写入读缓冲区的位置已经超出了缓冲区长度了
    {
        cout<<"read error at begin."<<endl;
        return false;
    }

    int bytes_read = 0;
    while(true)
    {
        bytes_read = recv(m_socket, m_read_buf + m_read_idx, READ_BUFF_SIZE - m_read_idx, 0);
        if(bytes_read == -1)    //出现错误
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            cout<<"bytes_read == -1"<<endl;
            return false;
        }
        else if (bytes_read == 0)
        {
            cout<<"bytes_read == 0"<<endl;
            return false;
            continue;
        }
        m_read_idx += bytes_read;
    }

    return true;
}

bool http_conn::write() //把响应的内容写到写缓冲区中,并説明该连接是否为长连接
{
    int bytes_write = 0;
    //先不考虑大文件的情况
    string response_head = "HTTP/1.1 200 OK\r\n\r\n";
    char head_temp[response_head.size()];
    strcpy(head_temp, response_head.c_str());
    m_iv[0].iov_base = head_temp;
    m_iv[0].iov_len = response_head.size() * sizeof(char);

    bytes_write = writev(m_socket, m_iv, m_iovec_length);

    if (bytes_write <= 0)
    {
        return false;
    }
    unmap();
    if (m_map["Connection"] == "keep-alive")
    {
        //return true;
        return false;
    }
    else
    {
        return false;
    }
}