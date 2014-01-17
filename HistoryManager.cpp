#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

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

void HistoryManager::getUserHistory(const Params &params)
{
    clean = false;
    updateShort = false;
    updateContext = false;

    key = params.getUserKey();

    getDeprecatedOffersAsync();

    getLongTermAsync();

    getShortTermAsync();

    getPageKeywordsAsync();

    getRetargetingAsync();

    //Запрос по П/З query
    std::string q = Params::getKeywordsString(params.getSearch());
    if (!q.empty())
    {
        lock();
        stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_query_,EBranchT::T1));
        unlock();
    }
    //Запрос по контексту страницы context
    q = Params::getContextKeywordsString(params.getContext());
    if (!q.empty())
    {
        lock();
        stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_context_,EBranchT::T2));
        unlock();
    }
}


void HistoryManager::sphinxProcess(Offer::Map &items)
{
    sphinx->makeFilter(items);

    getLongTermAsyncWait();
    getShortTermAsyncWait();
    getPageKeywordsAsyncWait();

    sphinx->processKeywords(stringQuery, items);
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
    updatePageKeywordsHistory(params.getContext());

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
