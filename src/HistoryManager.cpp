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
    pShortTerm = new SimpleRedisClient();
    pShortTerm->setHost(cfg->redis_short_term_history_host_.c_str());
    pShortTerm->setPort(strtol(cfg->redis_short_term_history_port_.c_str(),NULL,10));

    pLongTerm = new SimpleRedisClient();
    pLongTerm->setHost(cfg->redis_long_term_history_host_.c_str());
    pLongTerm->setPort(strtol(cfg->redis_long_term_history_port_.c_str(),NULL,10));

    pRetargeting = new SimpleRedisClient();
    pRetargeting->setHost(cfg->redis_retargeting_host_.c_str());
    pRetargeting->setPort(strtol(cfg->redis_retargeting_port_.c_str(),NULL,10));

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
