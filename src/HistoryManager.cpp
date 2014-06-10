#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

#define REDIS_TIMEOUT 3 * 24 * 3600

static const char * EnumHistoryTypeStrings[] = {"ShortTerm", "LongTerm", "ViewHistory", "Category","Retargeting"};

HistoryManager::HistoryManager(const std::string &tmpTableName):
    tmpTable(tmpTableName)
{
    module = Module_new();
    Module_init(module);

    m_pPrivate = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init((pthread_mutex_t*)m_pPrivate, &attr);
    pthread_mutexattr_destroy(&attr);

    tid = pthread_self();

    sphinx = new XXXSearcher();

    if(!isShortTerm() && !isLongTerm() && !isContext() && !isSearch())
    {
        std::clog<<"["<<tid<<"]"<<"no sphinx search enabled!"<<std::endl;
    }
}

HistoryManager::~HistoryManager()
{
    pthread_mutex_destroy((pthread_mutex_t*)m_pPrivate);

    delete sphinx;

    delete pViewHistory;
    delete pShortTerm;
    delete pLongTerm;
    delete pRetargeting;

    Module_free(module);
}

bool HistoryManager::initDB()
{

    Config *cfg = Config::Instance();

    pViewHistory = new RedisClient(cfg->redis_user_view_history_host_, cfg->redis_user_view_history_port_,Config::Instance()->views_expire_);
    pViewHistory->connect();

    pShortTerm = new RedisClient(cfg->redis_short_term_history_host_, cfg->redis_short_term_history_port_,REDIS_TIMEOUT);
    pShortTerm->connect();

    pLongTerm = new RedisClient(cfg->redis_long_term_history_host_, cfg->redis_long_term_history_port_,REDIS_TIMEOUT);
    pLongTerm->connect();

    pRetargeting = new RedisClient(cfg->redis_retargeting_host_, cfg->redis_retargeting_port_,REDIS_TIMEOUT);
    pRetargeting->connect();

    return true;
}

void HistoryManager::getUserHistory(Params *_params)
{
    std::string q;
    clean = false;
    updateShort = false;
    updateContext = false;

    params = _params;
    key = params->getUserKey();
    key_inv = key+"-inv";
    Log::gdb("key: %s",key.c_str());

    getDeprecatedOffersAsync();

    getRetargetingAsync();
//        std::clog<<"["<<tid<<"]"<< typeid(this).name()<<"::"<<__func__<<" no history for: "<<key<<std::endl;

    //Запрос по запросам к поисковикам
    if(isSearch())
    {
        q = getContextKeywordsString(params->getSearch());
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_search_,EBranchT::T1));
            unlock();
        }
    }
    //Запрос по контексту страницы
    if(isContext())
    {
        q = getContextKeywordsString(params->getContext());
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_context_,EBranchT::T2));
            unlock();
        }
    }

    if(isLongTerm())
        getLongTermAsync();

    if(isShortTerm())
        getShortTermAsync();
}

void HistoryManager::sphinxProcess(Offer::Map &items, float teasersMaxRating)
{

    if((!isShortTerm() && !isLongTerm() && !isContext() && !isSearch())
       || items.size() >= cfg->shpinx_min_offres_process_)
    {
        if(items.size() >= cfg->shpinx_min_offres_process_)
        {
            std::clog<<"shpinx_min_offres_process_: "
            <<cfg->shpinx_min_offres_process_<<" >= items count:"<<items.size()<<std::endl;
        }

        if(mtailOffers.size())
        {
            for(auto i = mtailOffers.begin(); i != mtailOffers.end(); ++i)
            {
                if(items[*i])
                {
                    items[*i]->rating = items[*i]->rating + teasersMaxRating;
                }
            }
        }
        return;
    }

    sphinx->makeFilter(items);

    if(isShortTerm() && !params->newClient)
        getShortTermAsyncWait();

    if(isLongTerm() && !params->newClient)
        getLongTermAsyncWait();

    Log::gdb("[%ld]sphinx get history: done",tid);

    sphinx->processKeywords(stringQuery, items);

    sphinx->cleanFilter();

    for(auto i = mtailOffers.begin(); i != mtailOffers.end(); ++i)
    {
        items[*i]->rating = items[*i]->rating + teasersMaxRating;
    }

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
                   append("longtermhistory", longTermArray)
                   //append("contexttermhistory", contextTermArray)
                   .obj()
                   ;
}

bool HistoryManager::updateUserHistory(
    const Offer::Vector &items,
    unsigned RetargetingCount)
{
    //обновление deprecated
    setDeprecatedOffers(items, RetargetingCount);
    //обновление retargeting
    RetargetingUpdate(items, RetargetingCount);

    vshortTerm.clear();
    vlongTerm.clear();
    vretageting.clear();
    vretg.clear();

    if(mtailOffers.size())
    {
        pViewHistory->del(key_inv);
        mtailOffers.clear();
    }

    stringQuery.clear();

    return true;
}

RedisClient *HistoryManager::getHistoryPointer(const HistoryType type) const
{
    switch(type)
    {
        case ViewHistory:
            return pViewHistory;
            break;
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
    RedisClient *r = getHistoryPointer(type);
    if(r->exists(key))
    {
        if(!r->getRange(key, 0, -1, rr))
        {
            std::clog<<LogPriority::Err<< "["<<tid<<"]"<< typeid(this).name()<<"::"<<__func__<<EnumHistoryTypeStrings[type]<<":"<< Module_last_error(module) << std::endl;
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
    return (myTimeAsInt%10000000000) ;
}

//------------------------------------------sync functions----------------------------------------
void HistoryManager::lock()
{
    pthread_mutex_lock((pthread_mutex_t*)m_pPrivate);
}

void HistoryManager::unlock()
{
    pthread_mutex_unlock((pthread_mutex_t*)m_pPrivate);
}
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
    if(!getHistoryPointer(t)->isConnected())
    {
        Log::err("HistoryManager::getDBStatus HistoryType: %d error: %s", (int)t, Module_last_error(module));
        return false;
    }
    return true;
}
