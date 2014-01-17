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
#include "XXXSearcher.h"

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
    std::list<std::string> vkeywords;
    std::list<std::string> vretageting;

    HistoryManager(const std::string &tmpTableName);
    virtual ~HistoryManager();

    /** \brief  Инициализация подключения к базам данных Redis
    */
    bool initDB();
    bool getDBStatus(HistoryType t);

    //main methods
    void getUserHistory(const Params &params);
    void sphinxProcess(Offer::Map &items);
    bool updateUserHistory(const Offer::Map &items, const Params& params);

    bool setDeprecatedOffers(const Offer::Map &items);
    bool getDeprecatedOffers(std::string &);
    bool getDeprecatedOffers();
    bool getDeprecatedOffersAsync();
    bool getDeprecatedOffersAsyncWait();
    bool clearDeprecatedOffers();

    bool getLongTermAsync();
    bool getLongTermAsyncWait();

    bool getShortTermAsync();
    bool getShortTermAsyncWait();

    bool getPageKeywordsAsync();
    bool getPageKeywordsAsyncWait();

    bool getRetargetingAsync();
    bool getRetargetingAsyncWait();


//    void getHistory();
protected:
private:
    std::string key;
    Module *module;
    std::map<HistoryType, RedisClient *> history_archive;
    std::string tmpTable;
    std::vector<sphinxRequests> stringQuery;
    XXXSearcher *sphinx;
    pthread_mutex_t *m_pPrivate;

    pthread_t   thrGetDeprecatedOffersAsync,
                thrGetLongTermAsync,
                thrGetShortTermAsync,
                thrGetPageKeywordsAsync,
                thrGetRetargetingAsync;

    bool getHistoryByType(HistoryType type, std::list<std::string> &rr);
    boost::int64_t currentDateToInt();
    void lock();
    void unlock();
    void getLongTerm();
    void getShortTerm();
    void getPageKeywordsHistory();
    void getRetargeting();

    void updateShortHistory(const std::string & query);
    void updatePageKeywordsHistory(const std::string & query);

    static void *getDeprecatedOffersEnv(void *);
    static void *getLongTermEnv(void *);
    static void *getShortTermEnv(void *);
    static void *getPageKeywordsEnv(void *);
    static void *getRetargetingEnv(void *);
};
#endif
