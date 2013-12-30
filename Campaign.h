#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <string>
#include <map>
#include <list>

#include "KompexSQLiteDatabase.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;


/**
  \brief  Класс, описывающий рекламную кампанию
*/
class Campaign
{
public:
    long id;
    std::string title;
    std::string project;
    bool social;
    bool valid;

    Campaign(long id);

    /** \brief Загружает информацию обо всех кампаниях */
    static void loadAll(Kompex::SQLiteDatabase *pdb);

private:

};

#endif // CAMPAIGN_H
