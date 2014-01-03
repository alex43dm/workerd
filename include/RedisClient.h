#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <string>
#include <vector>

#include <libredis/redis.h>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

class RedisClient
{
    public:
        int timeOutMSec;

        RedisClient(const std::string &host, const std::string &port);
        virtual ~RedisClient();

        bool connect();

        bool isConnected() const;

        bool addVal(const std::string &key, const std::string &member)
        {
            return _addVal(key, currentDateToInt(), member );
        }

        bool getRange(const std::string &key,
              int start,
              int stop,
              std::string &ret);

        bool exists(const std::string &key);
        long int zrank(const std::string &key, long id);
        bool zadd(const std::string &key, int score, long id);
        int zscore(const std::string &key, long id);
        bool zincrby(const std::string &key, long id, int inc);
        bool expire(const std::string &key, int time);

    protected:
    private:
        std::string host;
        std::string port;
        bool isConnected_;
        char *cmd;
        Connection *connection;

        bool _addVal(const std::string &key, double score, const std::string &member);
        boost::int64_t currentDateToInt();
        bool execCmd(const std::string &cmd);
};

#endif // REDISCLIENT_H
