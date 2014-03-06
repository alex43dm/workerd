#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <string>
#include <map>
#include <list>

#include "DB.h"
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
    int offersCount;

    Campaign(){};
    Campaign(long long id);

    /** \brief Загружает информацию в локальную бд
        обо всех кампаниях если фильтр - по умолчанию
        или о тех что заданы в фильтре
        \param[in] указатель на бд
        \param[in] фильтр mongo
    */
    static void loadAll(Kompex::SQLiteDatabase *pdb, mongo::Query=mongo::Query());
    /** \brief Обновить информацию в локальной бд (параметр 1) о кампании (параметр 2)*/
    static void update(Kompex::SQLiteDatabase *pdb, std::string aCampaignId);
    /** \brief Удалить кампанию(параметр 2) из локальной бд(параметр 1)*/
    static void remove(Kompex::SQLiteDatabase *pdb, std::string aCampaignId);
    /** \brief Остановить/запустить(парамерт 3) кампанию(параметр 2) guid*/
    static void startStop(Kompex::SQLiteDatabase *pdb, std::string aCampaignId, int);
    static std::string getName(long long campaign_id);
    static void GeoRerionsAdd(const std::string &country, const std::string &region);
    static void info(std::vector<Campaign*> &res);

private:

};

#endif // CAMPAIGN_H
