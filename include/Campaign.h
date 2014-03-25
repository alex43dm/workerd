#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <string>

#include "KompexSQLiteDatabase.h"

//typedef long long			sphinx_int64_t;
//typedef unsigned long long	sphinx_uint64_t;

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
    int offersCount;

    Campaign(){};
    Campaign(long long id);

    static std::string getName(long long campaign_id);
    static void info(std::vector<Campaign*> &res);

private:
};

#endif // CAMPAIGN_H
