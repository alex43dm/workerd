#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <vector>
#include <map>

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include "Offer.h"
#include "RedisClient.h"
#include "Params.h"

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

    HistoryManager();
    virtual ~HistoryManager();

    /** \brief  Инициализация подключения к базам данных Redis
    */
    bool initDB();
    void setKey(const std::string _key)
    {
        key = _key;
        clean = false;
        updateShort = false;
        updateContext = false;
    }
    bool getDBStatus(HistoryType t);
    bool updateUserHistory(const std::vector<Offer*> &items, const Params& params);

    bool setDeprecatedOffers(const std::vector<Offer*> &items);
    bool getDeprecatedOffers(std::string &);

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

private:
    std::string key;
    Module *module;
    std::map<HistoryType, RedisClient *> history_archive;

    void updateShortHistory(const std::string & query);
    void updateContextHistory(const std::string & query);
    bool getUserHistory(HistoryType type, std::list<std::string> &rr);

    boost::int64_t currentDateToInt();
};
#endif
