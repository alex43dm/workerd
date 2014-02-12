#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "DB.h"

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

    history_archive[ViewHistory] = new RedisClient(cfg->redis_user_view_history_host_, cfg->redis_user_view_history_port_);
    history_archive[ViewHistory]->connect();

    history_archive[Category] = new RedisClient(cfg->redis_category_host_, cfg->redis_category_port_);
    history_archive[Category]->connect();

    history_archive[ShortTerm] = new RedisClient(cfg->redis_short_term_history_host_, cfg->redis_short_term_history_port_);
    history_archive[ShortTerm]->connect();

    history_archive[LongTerm] = new RedisClient(cfg->redis_long_term_history_host_, cfg->redis_long_term_history_port_);
    history_archive[LongTerm]->connect();

    history_archive[PageKeywords] = new RedisClient(cfg->redis_page_keywords_host_, cfg->redis_page_keywords_port_);
    history_archive[PageKeywords]->connect();

    history_archive[Retargeting] = new RedisClient(cfg->redis_retargeting_host_, cfg->redis_retargeting_port_);
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

    getDeprecatedOffersAsync();

    getRetargetingAsync();

    //Запрос по запросам к поисковикам
    if (Config::Instance()->range_search_ > 0)
    {
        q = getKeywordsString(params->getSearch());
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_search_,EBranchT::T1));
            unlock();
        }
    }
    //Запрос по контексту страницы
    if (Config::Instance()->range_context_ > 0)
    {
        q = getContextKeywordsString(params->getContext());
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_context_,EBranchT::T2));
            unlock();
        }
    }

    if(Config::Instance()->range_long_term_ > 0)
        getLongTermAsync();

    if(Config::Instance()->range_short_term_ > 0)
        getShortTermAsync();

    if(Config::Instance()->range_context_ > 0)
        getPageKeywordsAsync();
}

void HistoryManager::sphinxProcess(Offer::Map &items, Offer::Vector &result)
{

    if(Config::Instance()->range_short_term_ <= 0
       && Config::Instance()->range_long_term_ <= 0
       && Config::Instance()->range_context_ <= 0
       && Config::Instance()->range_search_ <= 0
       )
    {
        return;
    }

    sphinx->makeFilter(items);

    if(Config::Instance()->range_short_term_ > 0)
        getShortTermAsyncWait();

    if(Config::Instance()->range_context_ > 0)
        getPageKeywordsAsyncWait();

    if(Config::Instance()->range_long_term_ > 0)
        getLongTermAsyncWait();

    Log::gdb("[%ld]sphinx get history: done",tid);

    sphinx->processKeywords(stringQuery, items, result);
}


/** Обновление short и deprecated историй пользователя. */
/** \brief  Обновление краткосрочной истории пользователя и истории его показов.
	\param offers     		вектор рекламных предложений, выбранных к показу
	\param params			параметры, переданный ядру процесса
*/
bool HistoryManager::updateUserHistory(
        const Offer::Vector &items,
        const Params *params,
        const Informer *informer)
{
    setDeprecatedOffers(items);

    RetargetingUpdate(items, informer->RetargetingCount);

    try
    {
        mongo::DB db("log");
        //LOG(INFO) << "writing to log...";

        int count = 0;
        std::list<std::string>::iterator it;

        mongo::BSONArrayBuilder b1,b2,b3;
        for (it=vshortTerm.begin() ; it != vshortTerm.end(); ++it )
            b1.append(*it);
        mongo::BSONArray shortTermArray = b1.arr();
        for (it=vlongTerm.begin() ; it != vlongTerm.end(); ++it )
            b2.append(*it);
        mongo::BSONArray longTermArray = b2.arr();
        for (it=vkeywords.begin() ; it != vkeywords.end(); ++it )
            b3.append(*it);
        mongo::BSONArray contextTermArray = b3.arr();

        for(auto i = items.begin(); i != items.end(); ++i)
        {

            std::tm dt_tm;
            dt_tm = boost::posix_time::to_tm(params->time_);
            mongo::Date_t dt( (mktime(&dt_tm)) * 1000LLU);

            mongo::BSONObj keywords = mongo::BSONObjBuilder().
                                      append("search", params->getSearch()).
                                      append("context", params->getContext()).
                                      append("ShortTermHistory", shortTermArray).
                                      append("longtermhistory", longTermArray).
                                      append("contexttermhistory", contextTermArray).
                                      obj();

            Campaign *c = new Campaign((*i)->campaign_id);

            mongo::BSONObj record = mongo::BSONObjBuilder().genOID().
                                    append("dt", dt).
                                    append("id", (*i)->id).
                                    append("id_int", (*i)->id_int).
                                    append("title", (*i)->title).
                                    append("inf", params->informer_).
                                    append("inf_int", informer->id).
                                    append("ip", params->ip_).
                                    append("cookie", params->cookie_id_).
                                    append("social", (*i)->social).
                                    append("token", (*i)->token).
                                    append("type", (*i)->type).
                                    append("isOnClick", (*i)->isOnClick).
                                    append("campaignId", c->id).
                                    append("campaignId_int", (*i)->campaign_id).
                                    append("campaignTitle", c->title).
                                    append("project", c->project).
                                    append("country", (params->getCountry().empty()?"NOT FOUND":params->getCountry().c_str())).
                                    append("region", (params->getRegion().empty()?"NOT FOUND":params->getRegion().c_str())).
                                    append("keywords", keywords).
                                    append("branch", (*i)->getBranch()).
                                    append("conformity", (*i)->conformity).
                                    append("matching", (*i)->matching).
                                    obj();
            delete c;

            db.insert("log.impressions", record, true);
            count++;

            offer_processed_ ++;
            if ((*i)->social) social_processed_ ++;
        }
    }
    catch (mongo::DBException &ex)
    {
        Log::err("DBException duriAMQPMessageng markAsShown(): %s", ex.what());
    }


    vshortTerm.clear();
    vlongTerm.clear();
    vkeywords.clear();
    vretageting.clear();

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
