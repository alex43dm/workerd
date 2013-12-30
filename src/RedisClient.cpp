#include <strings.h>

#include "RedisClient.h"
#include "Log.h"

#define CMD_SIZE 4096

RedisClient::RedisClient(const std::string &host, const std::string &port) :
    timeOutMSec(1000),
    host(host),
    port(port),
    isConnected_(false)
{
    //ctor
    cmd = new char[CMD_SIZE];
}

RedisClient::~RedisClient()
{
    //dtor
    delete []cmd;
    Connection_free(connection);
}

bool RedisClient::connect()
{
    connection = Connection_new((host + ":" + port).c_str());

    isConnected_ = true;
    return true;
}

bool RedisClient::getRange(const std::string &key,
                           int start,
                           int stop,
                           std::string &ret)
{

    Batch *batch;
    Executor *executor;
    /*= Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "EXISTS '%s'\r\n", key.c_str());
    Batch_write(batch, cmd, strlen(cmd), 1);

    Executor *executor = Executor_new();
    Executor_add(executor, connection, batch);

    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return false;
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
            printf("EXISTS: level: %d, reply type: %d, len: %ld data: '%s'\n", level, (int)reply_type, reply_len, reply_data);
            if(reply_type == RT_OK)
            {
                std::string ans = std::string(reply_data);
                if(ans.compare(0, 4, "PONG") == 0){ Batch_free(batch); return false; }
            }
        }
    }

    Batch_free(batch);
*/
    batch = Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZREVRANGE %s %d %d\r\n", key.c_str(), start, stop);
    Batch_write(batch, cmd, strlen(cmd), 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return false;
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
//            printf("ZREVRANGE level: %d, reply type: %d, len: %ld data: '%s'\n", level, (int)reply_type, reply_len, reply_data);
            if(RT_BULK == reply_type)
                ret += std::string(reply_data, reply_len) + ",";
        }
    }

    if (!ret.empty())
        ret.erase(std::prev(ret.end()));
/*
    if (!ret.empty())
        ret.erase(std::prev(ret.end()));

    if (!ret.empty())
        ret.erase(std::prev(ret.end()));
*/
    printf("ZREVRANGE ret: %s\n", ret.c_str());

    Batch_free(batch);

    return true;
}

bool RedisClient::_addVal(const std::string &key, double score, const std::string &member)
{
    Batch *batch = Batch_new();
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZADD %s %f %s\r\n", key.c_str(), score, member.c_str());
    Batch_write(batch, cmd, strlen(cmd), 1);

    printf("cmd: %s\n", cmd);

    Executor *executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return false;
    }

    Batch_free(batch);

    return false;
}

bool RedisClient::isConnected() const
{
    bool ret = false;
    Batch *batch = Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "PING\r\n");
    Batch_write(batch, cmd, strlen(cmd), 1);

    Executor *executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return false;
    }
    //read out replies
    ReplyType reply_type;
    char *reply_data;
    size_t reply_len;
    int level;
    while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
    {
        if(reply_type == RT_OK)
        {
            std::string ans = std::string(reply_data);
            if(ans.compare(0, 4, "PONG") == 0){ ret = true; break; }
        }
    }

    Batch_free(batch);
    return ret;
}

boost::int64_t RedisClient::currentDateToInt()
{
    boost::gregorian::date d(1970,boost::gregorian::Jan,1);
    boost::posix_time::ptime myTime = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime myEpoch(d);
    boost::posix_time::time_duration myTimeFromEpoch = myTime - myEpoch;
    boost::int64_t myTimeAsInt = myTimeFromEpoch.ticks();
    return (myTimeAsInt%10000000000) ;
}
