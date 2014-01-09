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

    bool HistoryManager::setDeprecatedOffers(const std::vector<Offer*> &items)
    {
        if(clean)
        {
            return history_archive[ViewHistory]->del(key);
        }

        for (auto it = items.begin(); it != items.end(); ++it)
        {
            if ((*it)->uniqueHits != -1)
            {
                if (history_archive[ViewHistory]->exists(key))
                {
                    if (history_archive[ViewHistory]->zrank(key,(*it)->id_int) == -1)
                    {
                        history_archive[ViewHistory]->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                    }
                    else//if rank == -1
                    {
                        if (history_archive[ViewHistory]->zscore(key,(*it)->id_int) > 0)
                        {
                            history_archive[ViewHistory]->zincrby(key, (*it)->id_int, -1);
                        }
                    }
                }
                else//if not exists
                {
                    history_archive[ViewHistory]->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
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
    bool HistoryManager::updateUserHistory(const std::vector<Offer*> &items, const Params& params)
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

    /** \brief  Инициализация весов для задания приоритететов групп рекламных предложений при формировании списка предложений к показу

    	\param range_priority_banners     	вес рекламных предложений типа "баннер по показам"
    	\param range_query 					вес рекламных предложений, соответствующих запросу с поисковика
    	\param range_shot_term 				вес рекламных предложений, соответствующих краткосрочной истории
    	\param range_long_term 				вес рекламных предложений, соответствующих долгосрочной истории
    	\param range_context 				вес рекламных предложений, соответствующих контексту страницы
    	\param range_context_term 			вес рекламных предложений, соответствующих истории контексту страницы
    	\param range_on_places 				вес рекламных предложений, типа "реклама на места размещения"

    */
    /*
    void initWeights(
        float range_priority_banners,
        float range_query,
        float range_shot_term,
        float range_long_term,
        float range_context,
        float range_context_term,
        float range_on_places
    )
    {
        w.range_priority_banners = range_priority_banners;
        w.range_query = range_query;
        w.range_shot_term = range_shot_term;
        w.range_long_term = range_long_term;
        w.range_context = range_context;
        w.range_context_term = range_context_term;
        w.range_on_places = range_on_places;
    }
    */

    /** \brief  Инициализация весов для задания приоритететов групп рекламных предложений при формировании списка предложений к показу

    		\param range_priority_banners     	вес рекламных предложений типа "баннер по показам"
    		\param range_query 					вес рекламных предложений, соответствующих запросу с поисковика
    		\param range_shot_term 				вес рекламных предложений, соответствующих краткосрочной истории
    		\param range_long_term 				вес рекламных предложений, соответствующих долгосрочной истории
    		\param range_context 				вес рекламных предложений, соответствующих контексту страницы
    		\param range_context_term 			вес рекламных предложений, соответствующих истории контексту страницы
    		\param range_on_places 				вес рекламных предложений, типа "реклама на места размещения"

    	*/
    /*
    void initWeights(
    const string &range_priority_banners,
    const string &range_query,
    const string &range_shot_term,
    const string &range_long_term,
    const string &range_context,
    const string &range_context_term,
    const string &range_on_places)
    {
    w.range_priority_banners = atof(range_priority_banners.c_str());
    w.range_query = atof(range_query.c_str());
    w.range_shot_term = atof(range_shot_term.c_str());
    w.range_long_term = atof(range_long_term.c_str());
    w.range_context = atof(range_context.c_str());
    w.range_context_term = atof(range_context_term.c_str());
    w.range_on_places = atof(range_on_places.c_str());
    }
    */

