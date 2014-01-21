#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <string>
#include <map>
#include <list>

#include <mongo/client/dbclientinterface.h>
#include "KompexSQLiteDatabase.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;


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

    Campaign(long long id);

    /** \brief Загружает информацию обо всех кампаниях */
    static void loadAll(Kompex::SQLiteDatabase *pdb, mongo::Query=mongo::Query());
    static void update(Kompex::SQLiteDatabase *pdb, std::string aCampaignId);
    static void remove(Kompex::SQLiteDatabase *pdb, std::string aCampaignId);
    static void startStop(Kompex::SQLiteDatabase *pdb, std::string aCampaignId, int);
    static std::string getName(long long campaign_id);
private:

};

#endif // CAMPAIGN_H
