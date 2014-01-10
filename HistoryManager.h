#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <list>
#include <map>

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "Offer.h"
#include "RedisClient.h"
#include "Params.h"
#include "SQLiteTmpTable.h"

typedef enum
{
    ShortTerm,
    LongTerm,
    ViewHistory,
    PageKeywords,
    Category,
    Retargeting
} HistoryType;


class HistoryManager
{
public:
    //Задаем значение очистки истории показов
    bool clean;
    //Задаем обнавление краткосрочной истории
    bool updateShort;
    //Задаём обнавление долгосрочной истории
    bool updateContext;

    std::list<std::string> vshortTerm;
    std::list<std::string> vlongTerm;
    std::list<std::string> vcontextTerm;

    HistoryManager();
    virtual ~HistoryManager();

    /** \brief  Инициализация подключения к базам данных Redis
    */
    bool initDB();

    void setParams(const Params& params);

    bool getDBStatus(HistoryType t);
    bool updateUserHistory(const Offer::Map &items, const Params& params);

    bool setDeprecatedOffers(const Offer::Map &items);
    bool getDeprecatedOffers(std::string &);
    bool getDeprecatedOffers();
    bool clearDeprecatedOffers();
/*
    bool getShortTerm(std::list<std::string> &r)
    {
        return getUserHistory(ShortTerm, r);
    }

    bool getLongTerm(std::list<std::string> &r)
    {
        return getUserHistory(LongTerm, r);
    }

    bool getPageKeywords(std::list<std::string> &r)
    {
        return getUserHistory(PageKeywords, r);
    }

    bool getCategory(std::list<std::string> &r)
    {
        return getUserHistory(Category, r);
    }

    bool getRetargeting(std::list<std::string> &r)
    {
        return getUserHistory(Retargeting, r);
    }
*/
private:
    std::string key;
    Module *module;
    std::map<HistoryType, RedisClient *> history_archive;
    SQLiteTmpTable *tmpTable;
    std::list <std::pair<std::string,std::pair<float,std::string>>> stringQuery;

    void updateShortHistory(const std::string & query);
    void updateContextHistory(const std::string & query);
    bool getUserHistory(HistoryType type, std::list<std::string> &rr);

    boost::int64_t currentDateToInt();
};
#endif
