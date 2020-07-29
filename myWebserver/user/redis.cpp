#include "redis.h"
#include <map>

redis_client *redis_client::m_redis_instance = new redis_client();

redis_client *redis_client::getinstance()
{
    return m_redis_instance;
}

string redis_client::getReply(string m_command)
{
    m_redis_lock.lock();
    m_redisReply = (redisReply *)redisCommand(m_redisContext, m_command.c_str());
    //cout << m_redisReply->type << endl;
    string temp = "";
    if (m_redisReply->elements == 0 && m_redisReply->type == 1)
    {
        //回复一行
        if (m_redisReply->len > 0)
        {
            temp = string(m_redisReply->str);
            //cout << "reply:   " << temp << endl;
        }
        else
            cout << "return nothing?" << endl;
    }
    else if (m_redisReply->type == 3)
    {
        //cout << "do code" << endl;
        int tempcode = m_redisReply->integer;
        temp = to_string(tempcode);
    }
    else
    {
        //for post
        temp += "{";
        for (int i = 0; i < m_redisReply->elements; ++i)
        {
            temp += "\"";
            temp += string(m_redisReply->element[i]->str);
            temp += "\"";
            if (i % 2 == 0)
                temp += ":";
            else
                temp += ",";
        }
        temp.pop_back();
        temp += "}";
    }
    freeReplyObject(m_redisReply);
    m_redis_lock.unlock();
    return temp;
}

redis_client::redis_client()
{
    struct timeval timeout = {2,0};
    m_redisContext = (redisContext *)redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    m_redisReply = NULL;
    board_exist();
}
string redis_client::getvoteboard()
{
    string board;
    board = getReply("zrange GOT 0 -1 withscores");
    //完整性校验
    if (board[0] == '{' && board[board.length() - 1] == '}')
    {
        return board;
    }
    else
    {
        return "";
    }
}
void redis_client::board_exist()
{
    string board;
    board = getReply("EXISTS GOT");
    //cout << board << endl;
    if (board != "1")
    {
        //cout << board << endl;
        //回头在vote环节增加校验，没有再添加，而不是直接初始化这些
        getReply("DEL GOT");
        getReply("zadd GOT 0 JohnSnow");
        getReply("zadd GOT 0 JaimeLannister");
        getReply("zadd GOT 0 NedStark");
        getReply("zadd GOT 0 TyrionLannister");
        getReply("zadd GOT 0 DaenerysTargaryen");
        getReply("zadd GOT 0 AryaStark");
        cout << "init ok" << endl;
    }
    //return board;
}