#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <string>
#include <list>

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

    //main methods
    void startGetUserHistory(Params *params, Informer *);
    void sphinxProcess(Offer::Map &items, float teasersMaxRating);
    bool updateUserHistory(const Offer::Map &items, const Offer::Vector &outItems, bool);


    bool moveUpTailOffers(Offer::Map &items, float teasersMaxRating);
    void getRetargetingAsyncWait();

    std::string isSphinxProcessed(){ return isProcessed;}

protected:
private:
    std::string isProcessed;
    std::string key;
    Params *params;
    Informer *inf;
    SimpleRedisClient *pShortTerm,*pLongTerm,*pRetargeting, *pCategory;

    XXXSearcher *sphinx;

    pthread_t   tid, thrGetDeprecatedOffersAsync,
                thrGetLongTermAsync,
                thrGetShortTermAsync,
                thrGetRetargetingAsync,
                thrGetTailOffersAsync,
                thrGetCategoryAsync;

    std::list<long> mtailOffers;

    unsigned minUniqueHits, maxUniqueHits;

    boost::int64_t currentDateToInt();
    void getLongTerm();
    void getShortTerm();
    void getRetargeting();
    void getCategory();

    void updateShortHistory(const std::string & query);

    static void *getDeprecatedOffersEnv(void *);
    static void *getLongTermEnv(void *);
    static void signalHanlerLongTerm(int);
    static void *getShortTermEnv(void *);
    static void signalHanlerShortTerm(int);
    static void *getRetargetingEnv(void *);
    static void signalHanlerRetargeting(int);
    static void *getTailOffersEnv(void *data);
    static void *getCategoryEnv(void *);
    static void signalHanlerCategory(int);
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

    bool getCategoryAsync();
    void getCategoryAsyncWait();
};
#endif
