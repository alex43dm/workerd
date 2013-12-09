/**
 Класс добавлен RealInvest Soft
 */
#pragma once

#include <string>
#include <vector>
#include <list>
#include "DBAccessor/DBWrapper.h"
#include "DBAccessor/DBConnectionParams.h"
#include "FullTextSearcher/SearcherWrapper.h"
#include "Params.h"
#include "Offer.h"
#include "Campaign.h"
#include "utils/benchmark.h"
#include "Core.h"

using namespace std;


/** \brief Класс, через который класс Core получает доступ к истории пользователя и осуществляет полнотекстовый поиск.

	Класс реализован по шаблону Singleton (Одиночка).


*/
class HistoryManager
{
private:
    HistoryManager() {}
protected:
    redisDB::DBWrapper accessor;	///< Объект доступа к базам данных Redis.
    SearcherWrapper searcher;		///< Объект доступа к библиотеке полнотекстового доступа.
    DBConnectionParams connParams;	///< Параматеры подключения к базам данных Redis.
    Weights w;						///< Весовые коэффициенты для задания приоритета историй при поиске.
public:
    /** \brief  Метод получения экземпляра класса

    	Метод создает единственный экземпляр класса при первом обращении. При повторных обращениях
    	возвращается ссылка на уже созданный объект.
    */
    static HistoryManager* instance()
    {
        static HistoryManager *ob = new HistoryManager;
        return ob;
    }

    /** \brief  Инициализация подключения к базам данных Redis

    	\param params     		параметры подключения
    */
    int initDB(DBConnectionParams &params)
    {
        connParams = params;
        return accessor.InitConnection(params);
    }

    /** \brief  Возвращает параметры подключения к базам данных Redis

    */
    DBConnectionParams* getConnectionParams()
    {
        return &connParams;
    }

    /** \brief  Возвращает статус подключения к одной из баз данных Redis.

    	\param t     		тип базы данных </BR>

    	Возможные значения:</BR>
    	1 - база данных краткосрочной истории</BR>
    	2 - база данных долгосрочной истории</BR>
    	3 - база данных истории показов</BR>

    	При некорректно заданном параметре метод вернет -1.
    */
    int getDBStatus(int t)
    {
        switch(t)
        {
        case 1:
            return accessor.getShortTermStatus();
        case 2:
            return accessor.getLongTermStatus();
        case 3:
            return accessor.getViewHistoryStatus();
        case 4:
            return accessor.getPageKeywordsStatus();
        case 5:
            return accessor.getCategoryStatus();
        case 6:
            return accessor.getRetargetingStatus();
        default:
            return -1;
        }
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


    void initLuceneIndexParams(string &folder_offer, string &folder_informer)
    {
        searcher.setIndexParams(folder_offer, folder_informer);
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


    /** \brief Получение идентификаторов РП от индекса lucene.
     *
     * Получение идентификаторов РП от индекса lucene.
     * \param params - параметры запроса.
     * \param campaigns - список кампаний, отобранных для показа.
     * \param campaignsAllGeo - список кампаний, отобранных для показа без учёта привязки к РБ.
     *
     * Возвращает список пар (идентификатор,вес), отсортированный по убыванию веса.
     *
     * В этом методе происходит обращение к redis за историей пользователя и отбор идентификаторов РП из индекса CLucene.
     */
    list<pair<pair<string, float>, pair<string, pair<string, string>>>> getOffersByUser1(const Params& params,
                                                                                         list<Campaign>& campaigns,
                                                                                         list<Campaign>& campaignsSoc,
                                                                                         list<Campaign>& campaignsAllGeo)
    {

        //uint64_t strr = Misc::currentTimeMillis();

        //получить историю пользователя от DBAccessor
        //UserHistory * uh = accessor.getUserHistoryByIdAndLocation(params.getUserKey(), params.getLocation());
        UserHistory * uh = accessor.getUserHistoryById(params.getUserKey());

        //// добавляем идентификаторы excluded РП в список deprecatedOffers
        uh->getDeprecatedOffers().insert(uh->getDeprecatedOffers().end(), params.excluded_offers_.begin(), params.excluded_offers_.end());

        //uint32_t srch = (int32_t)(Misc::currentTimeMillis() - strr);
        //LOG(INFO) << "\nAction before search took: " << srch << " ms.\n";


        list<pair<pair<string, float>, pair<string, pair<string, string>>>> result = searcher.f1(params.getSearch(), *uh, params.getContext(), params.getInformer(), campaigns, campaignsSoc, campaignsAllGeo, w);

        delete uh;

        return result;

    }


    /** \brief Получение краткосрочной истории пользователя.
     *
     * \param params - параметры запроса.
     */
    list<std::string> getShortHistoryByUser(const Params& params)
    {

        UserHistory * uh = accessor.getUserHistoryById(params.getUserKey());
        list <std::string> temp = uh->getShortHistory();
        delete uh;
        return temp;

    }
    /** \brief Получение долгосрочной истории пользователя.
     *
     * \param params - параметры запроса.
     */
    list<std::string> getLongHistoryByUser(const Params& params)
    {

        UserHistory * uh = accessor.getUserHistoryById(params.getUserKey());
        list <std::string> temp = uh->getLongHistory();
        delete uh;
        return temp;

    }
    /** \brief Получение контекстной истории пользователя.
     *
     * \param params - параметры запроса.
     */
    list<std::string> getContextHistoryByUser(const Params& params)
    {

        UserHistory * uh = accessor.getUserHistoryById(params.getUserKey());
        list <std::string> temp = uh->getContextHistory();
        delete uh;
        return temp;

    }
    /** Обновление short и deprecated историй пользователя. */
    /** \brief  Обновление краткосрочной истории пользователя и истории его показов.

    	\param offers     		вектор рекламных предложений, выбранных к показу
    	\param params			параметры, переданный ядру процесса
    */
    bool updateUserHistory(const vector<Core::ImpressionItem>& offers, const Params& params, bool clean, bool updateShort, bool updateContext)
    {
        if (clean)
        {
            accessor.cutDeprecatedUserHistory(params.getUserKey());
            //LOG(INFO) << "Очистка уникальности";
        }
        //Benchmark bench("\nвремя обновления историй пользователя (HistoryManager::updateUserHistory) ");
        //LOG(INFO) << "updateUserHistory start\n";
        accessor.updateDeprecatedUserHistory(params.getUserKey(), offers);
        //LOG(INFO) << "updateUserHistory middle\n";
        if (updateShort)
        {
            //LOG(INFO) << "updateShort";
            accessor.updateShortHistory(params.getUserKey(), params.getSearch());
        }
        if (updateContext)
        {
            //LOG(INFO) << "updateContext";
            accessor.updateContextHistory(params.getUserKey(), params.getContext());
        }
        //LOG(INFO) << "updateUserHistory end\n";
        return true;
    }
};
