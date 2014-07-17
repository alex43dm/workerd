#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

#define REDIS_EXPIRE 3 * 24 * 3600
#define SPHINX_CAPACITY_COUNT 2

static const char * EnumHistoryTypeStrings[] = {"ShortTerm", "LongTerm", "ViewHistory", "Category","Retargeting"};

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

    Config *cfg = Config::Instance();
/*
    pShortTerm = new RedisClient(cfg->redis_short_term_history_host_,
                                 cfg->redis_short_term_history_port_,
                                 REDIS_EXPIRE,
                                 cfg->redis_short_term_history_timeout_);
    pShortTerm->connect();

    pLongTerm = new RedisClient(cfg->redis_long_term_history_host_,
                                cfg->redis_long_term_history_port_,
                                REDIS_EXPIRE,
                                cfg->redis_long_term_history_timeout_);
    pLongTerm->connect();

    pRetargeting = new RedisClient(cfg->redis_retargeting_host_,
                                   cfg->redis_retargeting_port_,
                                   REDIS_EXPIRE,
                                   cfg->redis_retargeting_timeout_);
    pRetargeting->connect();
*/
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


/** Обновление short и deprecated историй пользователя. */
/** \brief  Обновление краткосрочной истории пользователя и истории его показов.
	\param offers     		вектор рекламных предложений, выбранных к показу
	\param params			параметры, переданный ядру процесса
*/
mongo::BSONObj HistoryManager::BSON_Keywords()
{
    std::list<std::string>::iterator it;
    mongo::BSONArrayBuilder b1,b2;//,b3;

    for (it=vshortTerm.begin() ; it != vshortTerm.end(); ++it )
        b1.append(*it);
    mongo::BSONArray shortTermArray = b1.arr();

    for (it=vlongTerm.begin() ; it != vlongTerm.end(); ++it )
        b2.append(*it);
    mongo::BSONArray longTermArray = b2.arr();
//        for (it=vkeywords.begin() ; it != vkeywords.end(); ++it )
//            b3.append(*it);
//        mongo::BSONArray contextTermArray = b3.arr();

    return         mongo::BSONObjBuilder().
                   append("ShortTermHistory", shortTermArray).
                   append("LongTermHistory", longTermArray)
                   //append("contexttermhistory", contextTermArray)
                   .obj()
                   ;
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

    vshortTerm.clear();
    vlongTerm.clear();
    vretageting.clear();

    for(auto p = vRISRetargetingResult.begin(); p != vRISRetargetingResult.end(); ++p)
    {
        if(*p)
        {
            delete (*p);
        }
    }

    vRISRetargetingResult.clear();

    if(mtailOffers.size())
    {
        mtailOffers.clear();
    }

    sphinx->cleanFilter();

    isProcessed = false;

    return true;
}

SimpleRedisClient *HistoryManager::getHistoryPointer(const HistoryType type) const
{
    switch(type)
    {
    case ShortTerm:
        return pShortTerm;
        break;
    case LongTerm:
        return pLongTerm;
        break;
    case Retargeting:
        return pRetargeting;
        break;
    default:
        return nullptr;
    }
}

/** \brief Получение истории пользователя.
 *
 * \param params - параметры запроса.
 */
bool HistoryManager::getHistoryByType(HistoryType type, std::list<std::string> &rr)
{
    SimpleRedisClient *r = getHistoryPointer(type);
    if(r->exists(key.c_str()))
    {
        if(!r->getRange(key, 0, -1, rr))
        {
            std::clog<<LogPriority::Err<< "["<<tid<<"]"<< typeid(this).name()<<"::"<<__func__<<EnumHistoryTypeStrings[type]<< std::endl;
            //Log::err("[%ld]%s::%s %s: %s", tid, typeid(this).name(), __func__,EnumHistoryTypeStrings[type], Module_last_error(module));
            return false;
        }
    }

    return true;
}


boost::int64_t HistoryManager::currentDateToInt()
{
    boost::gregorian::date d(1970,boost::gregorian::Jan,1);
    boost::posix_time::ptime myTime = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime myEpoch(d);
    boost::posix_time::time_duration myTimeFromEpoch = myTime - myEpoch;
    boost::int64_t myTimeAsInt = myTimeFromEpoch.ticks();

    return (myTimeAsInt%10000000000);
}

//------------------------------------------sync functions----------------------------------------
/** \brief  Возвращает статус подключения к одной из баз данных Redis.

	\param t     		тип базы данных </BR>

	Возможные значения:</BR>
	1 - база данных краткосрочной истории</BR>
	2 - база данных долгосрочной истории</BR>
	3 - база данных истории показов</BR>

	При некорректно заданном параметре метод вернет -1.
*/
bool HistoryManager::getDBStatus(HistoryType t)
{
    /*
    if(!getHistoryPointer(t)->isConnected())
    {
        std::clog<<"HistoryManager::getDBStatus HistoryType: "<<(int)t<<std::endl;
        return false;
    }
    */
    return true;
}
