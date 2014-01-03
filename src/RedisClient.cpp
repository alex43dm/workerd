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


    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZREVRANGE %s %d %d\r\n", key.c_str(), start, stop);

    batch = Batch_new();
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
            if(RT_BULK == reply_type)
                ret += std::string(reply_data, reply_len) + ",";
        }
    }

    if (!ret.empty())
        ret.erase(std::prev(ret.end()));

    Batch_free(batch);

    return true;
}

bool RedisClient::_addVal(const std::string &key, double score, const std::string &member)
{
    Batch *batch = Batch_new();
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZADD %s %f %s\r\n", key.c_str(), score, member.c_str());
    Batch_write(batch, cmd, strlen(cmd), 1);

//    printf("cmd: %s\n", cmd);

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

bool RedisClient::exists(const std::string &key)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "EXISTS '%s'\r\n", key.c_str());
    return execCmd(cmd);
}

long int RedisClient::zrank(const std::string &key, long id)
{
    long int ret;
    Batch *batch;
    Executor *executor;

    ret = -1;

    batch= Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZRANK %s %ld\r\n", key.c_str(), id);
    Batch_write(batch, cmd, strlen(cmd), 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);

    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return ret;
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
            if(reply_type == RT_INTEGER)
            {
                ret = strtol(reply_data,NULL,10);
                break;
            }
        }
    }

    Batch_free(batch);
    return ret;
}

bool RedisClient::zadd(const std::string &key, int score, long id)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZADD %s %d %ld\r\n", key.c_str(), score, id);
    return execCmd(cmd);
}

int RedisClient::zscore(const std::string &key, long id)
{
    int ret;
    Batch *batch;
    Executor *executor;

    ret = -1;

    batch= Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZSCORE %s %ld\r\n", key.c_str(), id);
    Batch_write(batch, cmd, strlen(cmd), 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);

    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return ret;
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
            if(reply_type == RT_INTEGER)
            {
                ret = strtol(reply_data,NULL,10);
                break;
            }
        }
    }

    Batch_free(batch);
    return ret;
}

bool RedisClient::zincrby(const std::string &key, long id, int inc)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZINCRBY %s %d %ld\r\n", key.c_str(), inc, id);
    return execCmd(cmd);
}

bool RedisClient::expire(const std::string &key, int time)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "EXPIRE %s %d\r\n", key.c_str(), time);
    return execCmd(cmd);
}

bool RedisClient::expire(const std::string &key, const std::string &time)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "EXPIRE %s %s\r\n", key.c_str(), time.c_str());
    return execCmd(cmd);
}

bool RedisClient::execCmd(const std::string &cmd)
{
    Batch *batch;
    Executor *executor;

    batch= Batch_new();

    Batch_write(batch, cmd.c_str(), cmd.size(), 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);

    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd.c_str());
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
            if(reply_type == RT_INTEGER)
            {
//                ret = strtol(reply_data,NULL,10);
                break;
            }
        }
    }

    Batch_free(batch);
    return true;
}

bool RedisClient::del(const std::string &key)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "DEL %s\r\n", key.c_str());
    return execCmd(cmd);
}
