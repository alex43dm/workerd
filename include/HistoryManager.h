#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <list>

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <mongo/bson/bson.h>

#include "Offer.h"
#include "Informer.h"
#include "Params.h"
#include "ParamParse.h"
#include "XXXSearcher.h"
#include "Config.h"
#include "SimpleRedisClient.h"

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
    void sphinxProcess(Offer::Map &items, float teasersMaxRating);
    bool updateUserHistory(const Offer::Map &items, const Offer::Vector &outItems, bool);


    bool moveUpTailOffers(Offer::Map &items, float teasersMaxRating);
    void getRetargetingAsyncWait();

    mongo::BSONObj BSON_Keywords();

    bool isSphinxProcessed(){ return isProcessed;}

protected:
private:
    bool isProcessed;
    std::string key;
    Params *params;
    Informer *inf;
    SimpleRedisClient *pShortTerm,*pLongTerm,*pRetargeting;

    XXXSearcher *sphinx;

    pthread_t   tid, thrGetDeprecatedOffersAsync,
                thrGetLongTermAsync,
                thrGetShortTermAsync,
                thrGetRetargetingAsync,
                thrGetTailOffersAsync;

    std::list<long> mtailOffers;

    unsigned minUniqueHits, maxUniqueHits;

    bool getHistoryByType(HistoryType type, std::list<std::string> &rr);
    boost::int64_t currentDateToInt();
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
    SimpleRedisClient *getHistoryPointer(const HistoryType type) const;

    bool getRetargetingAsync();
    void RISAlgorithmRetagreting(const Offer::MapRate &result);

    bool setDeprecatedOffers(const Offer::Vector &items);
    bool getDeprecatedOffers(std::string &);
    bool getDeprecatedOffers();
    bool getDeprecatedOffersAsync();
    bool getDeprecatedOffersAsyncWait();
    bool clearDeprecatedOffers();

    bool setTailOffers(const Offer::Map &items, const Offer::Vector &toShow, bool);
    bool getTailOffersAsyncWait();
    bool getTailOffersAsync();
    bool getTailOffers();

    bool getLongTermAsync();
    bool getLongTermAsyncWait();

    bool getShortTermAsync();
    bool getShortTermAsyncWait();
};
#endif
