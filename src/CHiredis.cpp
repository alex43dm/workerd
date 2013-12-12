#include <unistd.h>

#include "CHiredis.h"
#include "../Log.h"

CHiredis::CHiredis(const std::string &host, const std::string &port):
    host(host),
    port(atoi(port.c_str())),
    cntx(nullptr)
{

}

CHiredis::~CHiredis()
{
    if(cntx)
        redisFree(cntx);
}

bool CHiredis::connect()
{
    while((cntx = redisConnect(host.c_str(), port)) && cntx->err)
    {
        Log::err("CHiredis::connect %s", cntx->errstr);
        sleep(1);
    }

    return true;
}

bool CHiredis::getRange(const std::string &key,
                        int start,
                        int stop,
                        std::vector<std::string> &ret)
{
    if(!cntx)
        return false;

    redisReply *rlink;

    rlink = (redisReply *)redisCommand(cntx,"EXISTS %s", key.c_str());

    if(rlink->type != REDIS_REPLY_INTEGER)
    {
        freeReplyObject(rlink);
        return false;
    }

    freeReplyObject(rlink);

    rlink = (redisReply *)redisCommand(cntx,"ZREVRANGE %s %d %d", key.c_str(), start, stop);

    if(rlink->type != REDIS_REPLY_ARRAY)
    {
        freeReplyObject(rlink);
        return false;
    }

    for ( unsigned int i = 0; i < rlink->elements; i++ )
    {
        ret.push_back(rlink->element[i]->str);
    }

    freeReplyObject(rlink);
    return true;
}

bool CHiredis::_addVal(const std::string &key, double score, const std::string &member)
{
    if(!cntx)
        return false;
    redisReply *rlink;

    rlink = (redisReply *)redisCommand(cntx,"ZADD %s %f %s",
                                       key.c_str(), score, member.c_str());

    if(rlink->type != REDIS_REPLY_INTEGER && rlink->integer != 1)
        return false;

    freeReplyObject(rlink);
    return false;
}

boost::int64_t CHiredis::currentDateToInt()
{
    boost::gregorian::date d(1970,boost::gregorian::Jan,1);
    boost::posix_time::ptime myTime = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime myEpoch(d);
    boost::posix_time::time_duration myTimeFromEpoch = myTime - myEpoch;
    boost::int64_t myTimeAsInt = myTimeFromEpoch.ticks();
    return (myTimeAsInt%10000000000) ;
}
