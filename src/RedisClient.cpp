#include <strings.h>
#include <string.h>


#include "RedisClient.h"
#include "Log.h"
#include "Config.h"
#include "base64.h"

#define CMD_SIZE 4096

RedisClient::RedisClient(const std::string &host, const std::string &port, int expireTime) :
    timeOutMSec(1000),
    expireTime(expireTime),
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
    if(isConnected_)
    {
        Connection_free(connection);
    }
}

bool RedisClient::connect()
{
    isConnected_ = false;
    connection = Connection_new((host + ":" + port).c_str());
    if(connection==NULL)
    {
        Log::err("redis cannot connect to %s port %s", host.c_str(), port.c_str());
        return false;
    }
    isConnected_ = true;
    return true;
}

bool RedisClient::getRange(const std::string &key,
                           int start,
                           int stop,
                           std::string &resString)
{
    bool ret = true;
    Batch *batch;
    Executor *executor;

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZREVRANGEBYSCORE %s %d %d\r\n", key.c_str(), start, stop);

    batch = Batch_new();
    Batch_write(batch, cmd, strlen(cmd), 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);

    if(rr <= 0)
    {
        std::clog<<__func__<<" error: "<<cmd<<std::endl;
        ret = false;
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
            if(reply_type == RT_ERROR)
            {
                std::clog<<__func__<<" error: "<<reply_data<<std::endl;
                ret = false;
            }
            else if(RT_BULK == reply_type)
                resString += std::string(reply_data, reply_len) + ",";
        }
    }

    Executor_free(executor);
    Batch_free(batch);

    return ret;
}

bool RedisClient::getRange(const std::string &key,
                           int start,
                           int stop,
                           std::list<std::string> &retList)
{
    bool ret = true;
    Batch *batch;
    Executor *executor;

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZREVRANGEBYSCORE %s %d %d\r\n", key.c_str(), start, stop);

    batch = Batch_new();
    Batch_write(batch, cmd, strlen(cmd), 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);

    if(rr <= 0)
    {
        std::clog<<__func__<<" error: "<<cmd<<std::endl;
        ret = false;
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
            if(reply_type == RT_ERROR)
            {
                std::clog<<__func__<<" error: "<<reply_data<<std::endl;
                ret = false;
            }
            else if(RT_BULK == reply_type)
            {
                retList.push_back(std::string(reply_data, reply_len));
            }
        }
    }

    Executor_free(executor);
    Batch_free(batch);
    return ret;
}


bool RedisClient::getRange(const std::string &key,
                           int start,
                           int stop,
                           std::list<long> &ret)
{
    bool vret = false;
    Batch *batch;
    Executor *executor;

    bzero(cmd,CMD_SIZE);
    int rrr = snprintf(cmd, CMD_SIZE, "ZREVRANGEBYSCORE %s %d %d\r\n", key.c_str(), start, stop);

    batch = Batch_new();
    Batch_write(batch, cmd, rrr, 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);

    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
    }
    else
    {
        ReplyType reply_type;
        char *reply_data;
        size_t reply_len;
        int level;
        while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
        {
            if(reply_type == RT_ERROR)
            {
                Log::err("redis cmd: getRange false: %s", reply_data);
            }
            else if(RT_BULK == reply_type)
            {
                ret.push_back(strtol(reply_data,NULL,10));
            }

        }
        vret = true;
    }

    Executor_free(executor);
    Batch_free(batch);

    return vret;
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
        if(reply_type == RT_ERROR)
        {
            Log::err("redis cmd: isConnected false: %s", reply_data);
        }
        else if(reply_type == RT_OK)
        {
            std::string ans = std::string(reply_data);
            if(ans.compare(0, 4, "PONG") == 0){ ret = true; break; }
        }
    }

    Batch_free(batch);
    return ret;
}

bool RedisClient::exists(const std::string &key)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "EXISTS %s\r\n", key.c_str());
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
            if(reply_type == RT_ERROR)
            {
                Log::err("redis cmd: zrank false: %s",reply_data);
            }
            else if(reply_type == RT_INTEGER)
            {
                ret = strtol(reply_data,NULL,10);
                break;
            }
        }
    }

    Batch_free(batch);
    return ret;
}

bool RedisClient::zadd(const std::string &key, int64_t score, long id)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZADD %s %ld %ld\r\n", key.c_str(), score, id);
    execCmd(cmd);

    return expire(key, expireTime);
}

bool RedisClient::zadd(const std::string &key, int64_t score, const std::string &q)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZADD %s %ld %s\r\n", key.c_str(), score, q.c_str());
    execCmd(cmd);

    return expire(key, expireTime);
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
            if(reply_type == RT_ERROR)
            {
                Log::err("redis cmd: zscore false: %s", reply_data);
            }
            else
            {
                if(reply_type == RT_BULK)
                {
                    ret = atoi(reply_data);
                    break;
                }
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

bool RedisClient::expire(const std::string &key, long time)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "EXPIRE %s %ld\r\n", key.c_str(), time);
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
            if(reply_type == RT_ERROR)
            {
                Log::err("redis cmd: %s false: %s",cmd.c_str(), reply_data);
            }
            else if(reply_type == RT_INTEGER)
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

int RedisClient::zcount(const std::string &key) const
{
    int ret;
    Batch *batch;
    Executor *executor;

    ret = -1;

    batch= Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZSCORE %s 0 +inf\r\n", key.c_str());
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
            if(reply_type == RT_ERROR)
            {
                Log::err("redis cmd: zcount false: %s",reply_data);
            }
            else if(reply_type == RT_INTEGER)
            {
                ret = strtol(reply_data,NULL,10);
                break;
            }
        }
    }

    Batch_free(batch);
    return ret;
}

int RedisClient::zcount(const std::string &key, long Min, long Max) const
{
    int ret;
    Batch *batch;
    Executor *executor;

    ret = -1;

    batch= Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZSCORE %s %ld %ld\r\n", key.c_str(), Min, Max);
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
            if(reply_type == RT_ERROR)
            {
                Log::err("redis cmd: zcount false: %s",reply_data);
            }
            else if(reply_type == RT_INTEGER)
            {
                ret = strtol(reply_data,NULL,10);
                break;
            }
        }
    }

    Batch_free(batch);
    return ret;
}

bool RedisClient::zremrangebyrank(const std::string &key, int start, int stop)
{
    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "ZREMRANGEBYRANK %s %d %d\r\n", key.c_str(), start, stop);
    return execCmd(cmd);
}

bool RedisClient::set(const std::string &key, const std::string &val, long expireSeconds)
{
    bzero(cmd,CMD_SIZE);
    if(expireSeconds)
    {
        snprintf(cmd, CMD_SIZE, "SET %s %s EX %ld\r\n", key.c_str(), base64_encode(val).c_str(), expireSeconds);
    }
    else
    {
        snprintf(cmd, CMD_SIZE, "SET %s %s\r\n", base64_encode(key).c_str(), val.c_str());
    }

    execCmd(cmd);

    return true;
}

std::string RedisClient::get(const std::string &key)
{
    std::string ret;

    Batch *batch = Batch_new();

    bzero(cmd,CMD_SIZE);
    snprintf(cmd, CMD_SIZE, "GET %s\r\n", key.c_str());
    Batch_write(batch, cmd, strlen(cmd), 1);

    Executor *executor = Executor_new();
    Executor_add(executor, connection, batch);
    int rr = Executor_execute(executor, timeOutMSec);
    Executor_free(executor);
    if(rr <= 0)
    {
        Log::err("redis cmd false: %s",cmd);
        Batch_free(batch);
        return std::string();
    }
    //read out replies
    ReplyType reply_type;
    char *reply_data;
    size_t reply_len;
    int level;
    while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
    {
        if(reply_type == RT_ERROR)
        {
            Log::err("redis cmd: isConnected false: %s", reply_data);
        }
        else if(reply_type == RT_BULK)
        {
            ret = base64_decode(std::string(reply_data));
        }
    }

    Batch_free(batch);
    return ret;
}
