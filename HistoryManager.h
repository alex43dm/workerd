#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <vector>
#include <map>

#include "Offer.h"
#include "RedisClient.h"

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
    bool setDeprecatedOffers(const std::vector<Offer*> &items);
    bool getDeprecatedOffers(std::string &);

private:
    std::string key;
    Module *module;
    std::map<HistoryType, RedisClient *> history_archive;
};
#endif
