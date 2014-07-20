#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

#define REDIS_EXPIRE 3 * 24 * 3600
#define SPHINX_CAPACITY_COUNT 2

HistoryManager::HistoryManager()
{
    tid = pthread_self();

    sphinx = new XXXSearcher();
}

HistoryManager::~HistoryManager()
{
    delete sphinx;

    delete pShortTerm;
    delete pLongTerm;
    delete pRetargeting;

}

bool HistoryManager::initDB()
{
    pShortTerm = new SimpleRedisClient(cfg->redis_short_term_history_host_, cfg->redis_short_term_history_port_, "short");

    pLongTerm = new SimpleRedisClient(cfg->redis_long_term_history_host_,cfg->redis_long_term_history_port_, "long");

    pRetargeting = new SimpleRedisClient(cfg->redis_retargeting_host_,cfg->redis_retargeting_port_,"ret");

    if(cfg->logRedis)
    {
        pShortTerm->LogLevel(RC_LOG_DEBUG);
        pLongTerm->LogLevel(RC_LOG_DEBUG);
        pRetargeting->LogLevel(RC_LOG_DEBUG);
    }

    return true;
}

void HistoryManager::startGetUserHistory(Params *_params, Informer *inf_)
{
    clean = false;
    inf = inf_;
    params = _params;
    key = params->getUserKey();

    getRetargetingAsync();

    getTailOffersAsync();

    if( !inf->sphinxProcessEnable() )
    {
        return;
    }


    //Запрос по запросам к поисковикам

    std::string q;
    if(inf->isSearch())
    {
        sphinx->addRequest(params->getSearch(),inf->range_search,EBranchT::T1);
    }
    //Запрос по контексту страницы
    if(inf->isContext())
    {
        sphinx->addRequest(params->getContext(),inf->range_context,EBranchT::T2);
    }

    if(inf->isLongTerm())
    {
        getLongTermAsync();
    }

    if(inf->isShortTerm())
    {
        getShortTermAsync();
    }
}

void HistoryManager::sphinxProcess(Offer::Map &items, float teasersMaxRating)
{

    if( inf->capacity * SPHINX_CAPACITY_COUNT >= items.size() || !inf->sphinxProcessEnable() )
    {
        return;
    }

    isProcessed = true;

    if(inf->isShortTerm())
    {
        getShortTermAsyncWait();
    }

    if(inf->isLongTerm())
    {
        getLongTermAsyncWait();
    }

    sphinx->processKeywords(items, teasersMaxRating);
}



bool HistoryManager::updateUserHistory(
    const Offer::Map &items,
    const Offer::Vector &outItems,
    bool all_social)
{
    if(clean)
    {
        setTailOffers(items, outItems, all_social);
    }

    //обновление deprecated
    setDeprecatedOffers(outItems);
    //обновление retargeting

    for(auto p = vRISRetargetingResult.begin(); p != vRISRetargetingResult.end(); ++p)
    {
        if(*p)
        {
            delete (*p);
        }
    }

    vRISRetargetingResult.clear();

    mtailOffers.clear();

    sphinx->cleanFilter();

    isProcessed = false;

    return true;
}
