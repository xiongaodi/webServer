#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <string.h>
#include <iostream>
#include "../lock/lock.h"

using namespace std;

class redis_client
{
public:
    string setUserpasswd(string username, string passwd)
    {
        return getReply("set " + username + " " + passwd);
    }

    string getUserpasswd(string username)
    {
        return getReply("get " + username);
    }

    void vote(string votename)
    {
        if(votename.length() > 0)
        {
            string temp = getReply("ZINCRBY GOT 1" + votename); 
        }
    }

    string getvoteboard();
    void board_exist();

    static redis_client * getinstance();

private:
    locker m_redis_lock;                      //锁
    static redis_client *m_redis_instance;    //实例化对象
    struct timeval timeout;                   //超时
    redisContext *m_redisContext;             //用于保存连接状态
    redisReply *m_redisReply;

private:
    string getReply(string m_command); 
    redis_client();
};

#endif