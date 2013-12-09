#ifndef CHIREDIS_H
#define CHIREDIS_H

#include <string>
#include <vector>

#include <hiredis/hiredis.h>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

class CHiredis
{
    public:
        CHiredis(const std::string &host, const std::string &port);
        virtual ~CHiredis();

        bool connect();

        bool addVal(const std::string &key, const std::string &member)
        {
            return _addVal(key, currentDateToInt(), member );
        }

        bool getRange(const std::string &key,
              int start,
              int stop,
              std::vector<std::string> &ret);
    protected:
    private:
        std::string host;
        int port;
        redisContext *cntx;

        bool _addVal(const std::string &key, double score, const std::string &member);
        boost::int64_t currentDateToInt();
};

#endif // CHIREDIS_H
