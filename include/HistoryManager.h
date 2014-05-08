#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <list>
#include <map>

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <mongo/bson/bson.h>

#include "Offer.h"
#include "Informer.h"
#include "RedisClient.h"
#include "Params.h"
#include "ParamParse.h"
#include "XXXSearcher.h"
#include "Config.h"

typedef enum
{
    ShortTerm,
    LongTerm,
    ViewHistory,
    Category,
    Retargeting
} HistoryType;


class HistoryManager : public ParamParse
{
public:
    /// Счётчик обработанных запросов
    static int request_processed_;
    static int offer_processed_;
    static int social_processed_;
    //Задаем значение очистки истории показов
    bool clean;
    //Задаем обнавление краткосрочной истории
    bool updateShort;
    //Задаём обнавление долгосрочной истории
    bool updateContext;

    Offer::Vector *vretg;

    HistoryManager(const std::string &tmpTableName);
    virtual ~HistoryManager();

    /** \brief  Инициализация подключения к базам данных Redis
    */
    bool initDB();
    bool getDBStatus(HistoryType t);

    //main methods
    void getUserHistory(Params *params);
    void sphinxProcess(Offer::Map &items, float teasersMaxRating);
    bool updateUserHistory(const Offer::Vector &items, unsigned RetargetingCount);

    bool setDeprecatedOffers(const Offer::Vector &items);
    bool getDeprecatedOffers(std::string &);
    bool getDeprecatedOffers();
    bool getDeprecatedOffersAsync();
    bool getDeprecatedOffersAsyncWait();
    bool clearDeprecatedOffers();
    bool setTailOffers(const Offer::Map &items);

    bool getLongTermAsync();
    bool getLongTermAsyncWait();

    bool getShortTermAsync();
    bool getShortTermAsyncWait();

    bool getPageKeywordsAsync();
    bool getPageKeywordsAsyncWait();
    void *getPageKeywordsEnv(void *data);
    void getPageKeywordsHistory();

    bool getRetargetingAsync();
    void getRetargetingAsyncWait();
    void RetargetingUpdate(const Offer::Vector &v, unsigned);
    void RetargetingClear();

    bool isShortTerm(){return Config::Instance()->range_short_term_ > 0;}
    bool isLongTerm(){return Config::Instance()->range_long_term_ > 0;}
    bool isContext(){return Config::Instance()->range_context_ > 0;}
    bool isSearch(){return Config::Instance()->range_search_ > 0;}

    mongo::BSONObj BSON_Keywords();

protected:
private:
    std::string key, key_inv;
    Params *params;
    Module *module;
    std::map<HistoryType, RedisClient *> history_archive;
    std::string tmpTable;
    std::vector<sphinxRequests> stringQuery;
    XXXSearcher *sphinx;
    pthread_mutex_t *m_pPrivate;

    pthread_t   tid, thrGetDeprecatedOffersAsync,
                thrGetLongTermAsync,
                thrGetShortTermAsync,
                thrGetRetargetingAsync,
                thrGetPageKeywordsAsync;

    std::list<std::string> vshortTerm, vlongTerm, vretageting;
    std::list<long> mtailOffers;

    bool getHistoryByType(HistoryType type, std::list<std::string> &rr);
    boost::int64_t currentDateToInt();
    void lock();
    void unlock();
    void getLongTerm();
    void getShortTerm();
    void getRetargeting();

    void updateShortHistory(const std::string & query);

    static void *getDeprecatedOffersEnv(void *);
    static void *getLongTermEnv(void *);
    static void *getShortTermEnv(void *);
    static void *getRetargetingEnv(void *);
};
#endif
