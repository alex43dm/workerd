#include <strings.h>
#include <string.h>


#include "RedisClient.h"
#include "Log.h"
#include "Config.h"
#include "base64.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

#define CMD_SIZE 4096

RedisClient::RedisClient(const std::string &host, const std::string &port, int expireTime) :
    timeOutMSec(1000),
    expireTime(expireTime),
    host(host),
    port(port),
    connection(NULL)
{
    //ctor
    cmd = new char[CMD_SIZE];
}

RedisClient::~RedisClient()
{
    //dtor
    delete []cmd;

}

bool RedisClient::start(const std::string &Command)
{
    finish();

    connection = Connection_new((host + ":" + port).c_str());
    if(connection==NULL)
    {
        Log::err("RedisClient::connect error connect to %s port %s", host.c_str(), port.c_str());
        return false;
    }

    bzero(cmd,CMD_SIZE);
    int ret = snprintf(cmd, CMD_SIZE, "%s\r\n", Command.c_str());

    batch = Batch_new();
    Batch_write(batch, cmd, ret, 1);

    executor = Executor_new();
    Executor_add(executor, connection, batch);

    if(Executor_execute(executor, timeOutMSec) <= 0)
    {
        Log::err("RedisClient::%s %s false",__func__,Command.c_str());
        return false;
    }

    return true;
}

bool RedisClient::finish()
{
    if(connection!=NULL)
    {
        Connection_free(connection);
        connection = NULL;

        if(executor)
        {
            Executor_free(executor);
        }

        if(batch)
        {
            Batch_free(batch);
        }

        return true;
    }
    return false;
}

bool RedisClient::getRange(const std::string &key, const std::string &tableName)
{
    int cnt = 0;
    Kompex::SQLiteStatement *pStmt;

    if(!start("ZREVRANGEBYSCORE "+key+" 0 -1"))
    {
        return false;
    }

    pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

    while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
    {
        if(reply_type == RT_ERROR)
        {
            Log::err("RedisClient::%s cmd: getRange: %s",__func__,reply_data);
        }
        else if(RT_BULK == reply_type)
        {
            try
            {
                sqlite3_snprintf(CMD_SIZE, cmd, "INSERT INTO %s(id) VALUES(%ld);",
                                 tableName.c_str(),
                                 strtol(reply_data,NULL,10));
                pStmt->SqlStatement(cmd);

                cnt++;
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("SQLiteTmpTable::insert(%s) error: %s", ex.GetString().c_str());
            }

        }
        else
        {
            Log::warn("RedisClient::%s getRange: %s",__func__,reply_data);
        }
    }

    finish();

    delete pStmt;
    /*
        Kompex::SQLiteStatement *p;
        p = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
        try
        {
            sqlite3_snprintf(CMD_SIZE, cmd, "REINDEX idx_%s_id;",tableName.c_str());
            p->SqlStatement(cmd);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("DB error: REINDEX table: %s: %s",tableName.c_str(), ex.GetString().c_str());
        }
        delete p;
    */
    Log::gdb("RedisClient::%s: loaded %d",__func__,cnt);
    return status;
}



bool RedisClient::getRange(const std::string &key,
                           int start,
                           int stop,
                           std::string &ret)
{

    if(!RedisClient::start("ZREVRANGE "+key+" "+std::to_string(start)+" "+std::to_string(stop)))
    {
        return false;
    }

    while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
    {
        if(reply_type == RT_ERROR)
        {
            Log::err("redis getRange: false: %s", reply_data);
        }
        else if(RT_BULK == reply_type)
            ret += std::string(reply_data, reply_len) + ",";
    }

    if (!ret.empty())
        ret.erase(std::prev(ret.end()));

    finish();

    return true;
}

bool RedisClient::getRange(const std::string &key,
                           int start,
                           int stop,
                           std::list<std::string> &ret)
{
    if(!RedisClient::start("ZREVRANGE "+key+" "+std::to_string(start)+" "+std::to_string(stop)))
    {
        return false;
    }

    while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
    {
        if(reply_type == RT_ERROR)
        {
            Log::err("redis cmd: getRange false: %s", reply_data);
        }
        else if(RT_BULK == reply_type)
        {
            ret.push_back(std::string(reply_data, reply_len));
        }

    }

    return finish();
}

bool RedisClient::exists(const std::string &key)
{
    return execCmd("EXISTS "+key);
}

long int RedisClient::zrank(const std::string &key, long id)
{
    long int ret;
    ret = -1;

    if(!RedisClient::start("ZRANK "+key+" "+std::to_string(id)))
    {
        return false;
    }

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
    finish();
    return ret;
}

bool RedisClient::zadd(const std::string &key, int64_t score, long id)
{
    execCmd("ZADD "+key+" "+std::to_string(score)+" "+std::to_string(id));
    return expire(key, expireTime);
}

bool RedisClient::zadd(const std::string &key, int64_t score, const std::string &q)
{
    execCmd("ZADD "+key+" "+std::to_string(score)+" "+q);
    return expire(key, expireTime);
}

int RedisClient::zscore(const std::string &key, long id)
{
    int ret;
    ret = -1;

    if(!RedisClient::start("ZSCORE "+key+" "+std::to_string(id)))
    {
        return false;
    }
    while((level = Batch_next_reply(batch, &reply_type, &reply_data, &reply_len)))
    {
        if(reply_type == RT_ERROR)
        {
            Log::err("redis cmd: zscore false: %s", reply_data);
        }
        else if(reply_type == RT_INTEGER)
        {
            ret = strtol(reply_data,NULL,10);
            break;
        }
    }
    finish();
    return ret;
}

bool RedisClient::zincrby(const std::string &key, long id, int inc)
{
    return execCmd("ZINCRBY "+key+" "+std::to_string(inc)+" "+std::to_string(id));
}

bool RedisClient::expire(const std::string &key, long time)
{
    return execCmd("EXPIRE "+key+" "+std::to_string(time));
}

bool RedisClient::expire(const std::string &key, const std::string &time)
{
    return execCmd("EXPIRE "+key+" "+time);
}

bool RedisClient::execCmd(const std::string &cmd)
{
    if(!start(cmd))
    {
        return false;
    }

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
    finish();
    return true;
}

bool RedisClient::del(const std::string &key)
{
    return execCmd("DEL "+key);
}

int RedisClient::zcount(const std::string &key)
{
    int ret;
    ret = -1;

    if(!start("ZSCORE "+key+" 0 +inf"))
    {
        return false;
    }
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
    finish();
    return ret;
}

int RedisClient::zcount(const std::string &key, long Min, long Max)
{
    int ret;
    ret = -1;

    if(!start("ZSCORE "+key+" "+std::to_string(Min)+" "+std::to_string(Max)))
    {
        return false;
    }

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
    finish();
    return ret;
}

bool RedisClient::zremrangebyrank(const std::string &key, int start, int stop)
{
    return execCmd("ZREMRANGEBYRANK "+key+" "+std::to_string(start)+" "+std::to_string(stop));
}

bool RedisClient::set(const std::string &key, const std::string &val, long expireSeconds)
{
    if(expireSeconds)
    {
        return execCmd("SET "+key+" "+base64_encode(val)+" EX "+std::to_string(expireSeconds));
    }
    else
    {
        return execCmd("SET "+key+" "+base64_encode(val));
    }
}

std::string RedisClient::get(const std::string &key)
{
    std::string ret;

    if(!start("GET "+key))
    {
        return std::string();
    }

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

    finish();
    return ret;
}
