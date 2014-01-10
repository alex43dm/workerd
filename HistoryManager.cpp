#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

HistoryManager::HistoryManager()
{
    module = Module_new();
    Module_init(module);
}

HistoryManager::~HistoryManager()
{
    Module_free(module);
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

void HistoryManager::setParams(const Params& params)
{
    key = params.getUserKey();
    clean = false;
    updateShort = false;
    updateContext = false;

/*
    //Запрос по П/З query
    std::string q = Params::getKeywordsString(params.getSearch());
    if (!q.empty())
    {
        stringQuery.push_back(std::pair<std::string,std::pair<float,std::string>>(
            q, std::pair<float,std::string>(Config::Instance()->range_query_,"T1")));
    }
    //Запрос по контексту страницы context
    q = Params::getContextKeywordsString(params.getContext());
    if (!q.empty())
    {
        stringQuery.push_back(std::pair<std::string,std::pair<float,std::string>>(
            q, std::pair<float,std::string>(Config::Instance()->range_context_,"T2")));
    }

    std::list<std::string> vList;

    getUserHistory(HistoryType::ShortTerm, vList);

    for (auto i=vList.begin(); i != vList.end(); ++i)
    {
        std::string strSH = *i;
        if (!strSH.empty())
        {
            q = Params::getKeywordsString(strSH);
            if (!q.empty())
            {
                stringQuery.push_back(std::pair<std::string,std::pair<float,std::string>>(
                    q, std::pair<float,std::string>(Config::Instance()->range_short_term_,"T3")));
            }
        }
    }

    vList.clear();

    getUserHistory(HistoryType::PageKeywords, vList);

    for (auto i=vList.begin(); i != vList.end(); ++i)
    {
        std::string strSH = *i;
        if (!strSH.empty())
        {
            q = Params::getContextKeywordsString(strSH);
            if (!q.empty())
            {
                stringQuery.push_back(std::pair<std::string,std::pair<float,std::string>>(
                    q, std::pair<float,std::string>(Config::Instance()->range_context_term_,"T4")));
            }
        }
    }

    vList.clear();

    getUserHistory(HistoryType::LongTerm, vList);

    for (auto i=vList.begin(); i != vList.end(); ++i)
    {
        std::string strSH = *i;
        if (!strSH.empty())
        {
            q = Params::getKeywordsString(strSH);
            if (!q.empty())
            {
                stringQuery.push_back(std::pair<std::string,std::pair<float,std::string>>(
                    q, std::pair<float,std::string>(Config::Instance()->range_long_term_,"T5")));
            }
        }
    }

    vList.clear();
*/
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

bool HistoryManager::clearDeprecatedOffers()
{
    return history_archive[ViewHistory]->del(key);
}

bool HistoryManager::setDeprecatedOffers(const Offer::Map &items)
{
    if(clean)
    {
        return history_archive[ViewHistory]->del(key);
    }

    for (Offer::cit it = items.begin(); it != items.end(); ++it)
    {
        if (it->second->uniqueHits != -1)
        {
            if (history_archive[ViewHistory]->exists(key))
            {
                if (history_archive[ViewHistory]->zrank(key,it->second->id_int) == -1)
                {
                    history_archive[ViewHistory]->zadd(key,it->second->uniqueHits - 1, it->second->id_int);
                }
                else//if rank == -1
                {
                    if (history_archive[ViewHistory]->zscore(key,it->second->id_int) > 0)
                    {
                        history_archive[ViewHistory]->zincrby(key, it->second->id_int, -1);
                    }
                }
            }
            else//if not exists
            {
                history_archive[ViewHistory]->zadd(key,it->second->uniqueHits - 1, it->second->id_int);
                history_archive[ViewHistory]->expire(key, Config::Instance()->views_expire_);
            }
        }
    }
    return true;
}

/** \brief Получение идентификаторов РП от индекса lucene.
 *
 * Возвращает список пар (идентификатор,вес), отсортированный по убыванию веса.
 *
 * В этом методе происходит обращение к redis за историей пользователя и отбор идентификаторов РП из индекса CLucene.
 */
bool HistoryManager::getDeprecatedOffers(std::string &rr)
{

    if(!history_archive[ViewHistory]->getRange(key , 0, -1, rr))
    {
        Log::err("HistoryManager::getDeprecatedOffers error: %s", Module_last_error(module));
        return false;
    }

    return true;
}

bool HistoryManager::getDeprecatedOffers()
{

    if(!history_archive[ViewHistory]->getRange(key))
    {
        Log::err("HistoryManager::getDeprecatedOffers error: %s", Module_last_error(module));
        return false;
    }

    return true;
}

/** \brief Получение истории пользователя.
 *
 * \param params - параметры запроса.
 */
bool HistoryManager::getUserHistory(HistoryType type, std::list<std::string> &rr)
{

    if(!history_archive[type]->getRange(key , 0, -1, rr))
    {
        Log::err("HistoryManager::getUserHistory error: %s", Module_last_error(module));
        return false;
    }

    return true;
}

/** Обновление short и deprecated историй пользователя. */
/** \brief  Обновление краткосрочной истории пользователя и истории его показов.
	\param offers     		вектор рекламных предложений, выбранных к показу
	\param params			параметры, переданный ядру процесса
*/
bool HistoryManager::updateUserHistory(const Offer::Map &items, const Params& params)
{
    setDeprecatedOffers(items);
    updateShortHistory(params.getSearch());
    updateContextHistory(params.getContext());
    return true;
}

void HistoryManager::updateShortHistory(const std::string & query)
{
    if(query.empty() && !updateShort)
        return;

    history_archive[ShortTerm]->zadd(key, currentDateToInt(), query);
    history_archive[ShortTerm]->expire(key, Config::Instance()->shortterm_expire_);
    if (history_archive[ShortTerm]->zcount(key) >= 3)
    {
        history_archive[ShortTerm]->zremrangebyrank(key, 0, 0);
    }
}

void HistoryManager::updateContextHistory(const std::string & query)
{
    if(query.empty() && !updateContext)
        return;

    history_archive[PageKeywords]->zadd(key, currentDateToInt(), query);
    history_archive[PageKeywords]->expire(key, Config::Instance()->context_expire_);
    if (history_archive[PageKeywords]->zcount(key) >= 3)
    {
        history_archive[PageKeywords]->zremrangebyrank(key, 0, 0);
    }
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
