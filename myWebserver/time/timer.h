#ifndef TIMER_H
#define TIMER_H

#include<time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <iostream>
using namespace std;

class util_timer;
class client_data
{
public: 
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};

class util_timer
{
public: 
    util_timer():prev(NULL), next(NULL){}

public: 
    time_t  livetime;
    void (*cb_func)(client_data *);
    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

class sort_timer_list
{
private:
    void add_timer(util_timer * timer, util_timer *lst_head);

public:
    sort_timer_list():head(NULL), tail(NULL){}
    ~sort_timer_list();

    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();

    util_timer *remove_from_list(util_timer *timer);

public:
    util_timer *head;
    util_timer *tail;
};

sort_timer_list::~sort_timer_list()
{
    util_timer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_list::tick()    //对于列表中的时间事件进行遍历，清空超时的
{
    if(!head)
        return;
    time_t cur = time(NULL);
    util_timer * tmp = head;
    while(tmp)
    {
        if(cur < tmp->livetime)  // 如果当前的时间还没到他的关闭时间
        {
            break;
        }
        else 
        {
            tmp->cb_func(tmp->user_data);   //关闭连接
        }

        head = tmp->next;
        if(head)
            head ->prev = NULL;
        delete tmp;
        tmp = head;
    }
    
}

void sort_timer_list::add_timer(util_timer *timer)
{
    if(!timer)
    {
        cout<<"not a timer."<<endl;
        return;
    }
    if(!head)
    {
        head = timer;
        tail = timer;
        return;
    }

    if(timer->livetime < head->livetime)    //如果当前定时器的时间小于头部的定时器时间（更早触发）
    {
        //那就把它放到前面，最早清理
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    else 
    {
         //放到头部后面的合适的位置
         util_timer *tmp = head;
         while(tmp)
         {
             if(tmp->livetime > timer->livetime)
             {
                 //如果找到了位置，则进行插入
                 util_timer *tmppre = timer->prev;
                 timer->prev = tmp->prev;
                 timer->next = tmp;
                 tmppre->next = timer;
                 tmp->prev = timer;
                 return;
             }
         }

         //插入到尾部后面
         tail->next = timer;
         timer->prev = tail;
         tail = timer;
         return;
    }
}

util_timer *sort_timer_list::remove_from_list(util_timer *timer)
{
    if (!timer)
        return timer;
    else if ((timer == head) && (timer == tail))
        head = tail = NULL;
    else if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
    }
    else if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        timer->prev = NULL;
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        timer->prev = NULL;
        timer->next = NULL;
    }
    return timer;
}
void sort_timer_list::del_timer(util_timer *timer) //对于列表中的时间事件删除,参数中的timer对应的描述符已经关闭了
{
    if (!timer)
        return;
    remove_from_list(timer);
    delete timer;
}

void sort_timer_list::adjust_timer(util_timer *timer) //对于列表中的时间事件进行按照时间顺序的调整再重新插入
{
    if (timer)
    {
        util_timer *temp = remove_from_list(timer);
        //cout << "remove ok" << endl;
        add_timer(temp);
        //cout << "add ok" << endl;
    }

}


#endif