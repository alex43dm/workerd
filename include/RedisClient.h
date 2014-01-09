#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <string>
#include <vector>
#include <list>

#include <libredis/redis.h>

class RedisClient
{
    public:
        int timeOutMSec;

        RedisClient(const std::string &host, const std::string &port);
        virtual ~RedisClient();

        bool connect();

        bool isConnected() const;
/*
        bool addVal(const std::string &key, const std::string &member)
        {
            return _addVal(key, currentDateToInt(), member );
        }
*/
        bool getRange(const std::string &key,
              int start,
              int stop,
              std::string &ret);

        bool getRange(const std::string &key,
              int start,
              int stop,
              std::list<std::string> &ret);

        bool getRange(const std::string &key);

        bool exists(const std::string &key);
        long int zrank(const std::string &key, long id);
        bool zadd(const std::string &key, int64_t score, long id);
        bool zadd(const std::string &key, int64_t score, const std::string &q);
        int zscore(const std::string &key, long id);
        bool zincrby(const std::string &key, long id, int inc);
        bool expire(const std::string &key, int time);
        bool expire(const std::string &key, const std::string &time);
        bool del(const std::string &key);
        int zcount(const std::string &key) const;
        int zcount(const std::string &key, long Min, long Max) const;
        bool zremrangebyrank(const std::string &key, int start, int stop);

    protected:
    private:
        std::string host;
        std::string port;
        bool isConnected_;
        char *cmd;
        Connection *connection;

        bool _addVal(const std::string &key, double score, const std::string &member);
        bool execCmd(const std::string &cmd);
};

#endif // REDISCLIENT_H
