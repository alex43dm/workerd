#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <list>

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

/*
const char * getTextEnumHistory( int enumVal )
{
  return EnumHistoryTypeStrings[enumVal];
}
*/
class HistoryManager : public ParamParse
{
public:
    /// Счётчик обработанных запросов
    static int request_processed_;
    static int offer_processed_;
    static int social_processed_;
    //Задаем значение очистки истории показов
    bool clean;

    Offer::Vector vRISRetargetingResult;

    HistoryManager();
    virtual ~HistoryManager();

    /** \brief  Инициализация подключения к базам данных Redis
    */
    bool initDB();
    bool getDBStatus(HistoryType t);

    //main methods
    void startGetUserHistory(Params *params, Informer *);
    void sphinxProcess(Offer::Map &items);
    bool updateUserHistory(const Offer::Map &items, const Offer::Vector &outItems, bool);

    bool setDeprecatedOffers(const Offer::Vector &items);
    bool getDeprecatedOffers(std::string &);
    bool getDeprecatedOffers();
    bool getDeprecatedOffersAsync();
    bool getDeprecatedOffersAsyncWait();
    bool clearDeprecatedOffers();

    bool setTailOffers(const Offer::Map &items, const Offer::Vector &toShow, bool);
    bool getTailOffersAsyncWait();
    bool getTailOffersAsync();
    bool moveUpTailOffers(Offer::Map &items, float teasersMaxRating);
    bool getTailOffers();

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
    void RetargetingUpdate(const Offer::Vector &);
    void RetargetingClear();
    unsigned RetargetingCount() const { return vRISRetargetingResult.size(); }

    mongo::BSONObj BSON_Keywords();

    bool isSphinxProcessed(){ return isProcessed;}

protected:
private:
    bool isProcessed;
    std::string key;
    Params *params;
    Informer *inf;
    RedisClient *pShortTerm,*pLongTerm,*pRetargeting;
    std::vector<sphinxRequests> stringQuery;
    XXXSearcher *sphinx;

    pthread_mutex_t *m_pPrivate;

    pthread_t   tid, thrGetDeprecatedOffersAsync,
                thrGetLongTermAsync,
                thrGetShortTermAsync,
                thrGetRetargetingAsync,
                thrGetPageKeywordsAsync,
                thrGetTailOffersAsync;

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
    static void signalHanlerLongTerm(int);
    static void *getShortTermEnv(void *);
    static void signalHanlerShortTerm(int);
    static void *getRetargetingEnv(void *);
    static void signalHanlerRetargeting(int);
    static void *getTailOffersEnv(void *data);
    RedisClient *getHistoryPointer(const HistoryType type) const;

    void RISAlgorithmRetagreting(const Offer::MapRate &result);
};
#endif
