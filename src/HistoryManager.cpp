#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

#define REDIS_TIMEOUT 3 * 24 * 3600

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
}

HistoryManager::~HistoryManager()
{
    pthread_mutex_destroy((pthread_mutex_t*)m_pPrivate);
    Module_free(module);
    delete sphinx;
}

bool HistoryManager::initDB()
{

    Config *cfg = Config::Instance();

    history_archive[ViewHistory] = new RedisClient(cfg->redis_user_view_history_host_, cfg->redis_user_view_history_port_,Config::Instance()->views_expire_);
    history_archive[ViewHistory]->connect();

    history_archive[ShortTerm] = new RedisClient(cfg->redis_short_term_history_host_, cfg->redis_short_term_history_port_,REDIS_TIMEOUT);
    history_archive[ShortTerm]->connect();

    history_archive[LongTerm] = new RedisClient(cfg->redis_long_term_history_host_, cfg->redis_long_term_history_port_,REDIS_TIMEOUT);
    history_archive[LongTerm]->connect();

    history_archive[Retargeting] = new RedisClient(cfg->redis_retargeting_host_, cfg->redis_retargeting_port_,REDIS_TIMEOUT);
    history_archive[Retargeting]->connect();

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

    if(!isShortTerm() && !isLongTerm() && !isContext() && !isSearch())
    {
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

    if(isShortTerm())
        getShortTermAsyncWait();

    if(isLongTerm())
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
    setDeprecatedOffers(items);

    RetargetingUpdate(items, RetargetingCount);

    vshortTerm.clear();
    vlongTerm.clear();
    vretageting.clear();

    if(mtailOffers.size())
    {
        history_archive[ViewHistory]->del(key_inv);
        mtailOffers.clear();
    }

    stringQuery.clear();

    return true;
}

/** \brief Получение истории пользователя.
 *
 * \param params - параметры запроса.
 */
bool HistoryManager::getHistoryByType(HistoryType type, std::list<std::string> &rr)
{

    if(!history_archive[type]->getRange(key , 0, -1, rr))
    {
        Log::err("HistoryManager::getHistoryByType error: %s", Module_last_error(module));
        return false;
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
    if(!history_archive[t]->isConnected())
    {
        Log::err("HistoryManager::getDBStatus HistoryType: %d error: %s", (int)t, Module_last_error(module));
        return false;
    }
    return true;
}
