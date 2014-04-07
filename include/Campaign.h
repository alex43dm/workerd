#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <string>

#include "KompexSQLiteDatabase.h"

//typedef long long			sphinx_int64_t;
//typedef unsigned long long	sphinx_uint64_t;

enum class showCoverage : std::int8_t { all, allowed, thematic };

/**
  \brief  Класс, описывающий рекламную кампанию
*/
class Campaign
{
public:
    long long id;
    std::string guid;
    std::string title;
    std::string project;
    bool social;
    bool valid;
    showCoverage type;
    int offersCount;

    Campaign(){};
    Campaign(long long id);

    static showCoverage typeConv(const std::string &t)
    {
        if( t == "all")
        {
            return showCoverage::all;
        }
        else if( t == "allowed")
        {
            return showCoverage::allowed;
        }
        else if( t == "thematic")
        {
            return showCoverage::thematic;
        }
        else
        {
            return showCoverage::all;
        }
    }

    void setType(const std::string &t)
    {
        if( t == "all")
        {
            type = showCoverage::all;
        }
        else if( t == "allowed")
        {
            type = showCoverage::allowed;
        }
        else if( t == "thematic")
        {
            type = showCoverage::thematic;
        }
        else
        {
            type = showCoverage::all;
        }
    }

    static std::string getName(long long campaign_id);
    static void info(std::vector<Campaign*> &res);

private:
};

#endif // CAMPAIGN_H
