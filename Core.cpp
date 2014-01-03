#include "DB.h"
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <cstdlib>
#include <mongo/util/version.h>
#include <mongo/bson/bsonobjbuilder.h>

#include "Config.h"
#include "Log.h"
#include "Core.h"
#include "InformerTemplate.h"
#include "utils/Comparators.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "utils/base64.h"

using std::list;
using std::vector;
using std::map;
using std::unique_ptr;
using std::string;
using namespace boost::posix_time;
using namespace Kompex;
#define foreach	BOOST_FOREACH

#define CMD_SIZE 8192

int Core::request_processed_ = 0;
int Core::offer_processed_ = 0;
int Core::social_processed_ = 0;

Core::Core(DataBase *_pDb) :
    redirect_script_("/redirect"),
    pDb(_pDb)
{
    tid = pthread_self();

    cmd = new char[CMD_SIZE];

    pStmtOfferStr = pDb->getSqlFile("requests/01.sql");
/*
    try
    {
        pStmtOffer = new SQLiteStatement(pDb->pDatabase);
        pStmtOffer->Sql(pDb->getSqlFile("requests/01.sql"));
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: pStmtOffer: %s %s", ex.GetString().c_str(), pDb->getSqlFile("requests/01.sql").c_str());
        exit(1);
    }
*/
    try
    {
        pStmtInformer = new SQLiteStatement(pDb->pDatabase);
        pStmtInformer->Sql(pDb->getSqlFile("requests/02.sql"));
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: pStmtInformer: %s %s", ex.GetString().c_str(), pDb->getSqlFile("requests/02.sql").c_str());
        exit(1);
    }


    try
    {
        pStmtOfferDefault = new SQLiteStatement(pDb->pDatabase);
        pStmtOfferDefault->Sql(pDb->getSqlFile("requests/default.sql"));
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: pStmtOfferDefault: %s: %s", ex.GetString().c_str(), pDb->getSqlFile("requests/default.sql").c_str());
        exit(1);
    }

    hm = new HistoryManager();
    hm->initDB();

    Log::info("[%ld]core start",tid);
}

Core::~Core()
{
    delete []cmd;

    pStmtInformer->FreeQuery();
    delete pStmtInformer;

    pStmtOffer->FreeQuery();
    delete pStmtOffer;

    pStmtOfferDefault->FreeQuery();
    delete pStmtOfferDefault;

    delete pDb;
    delete hm;
}

/** Функтор составляет ссылку перенаправления на предложение item.

    Вся строка запроса кодируется в base64 и после распаковки содержит
    следующие параметры:

    \param id        guid рекламного предложения
    \param inf       guid информера, с которого осуществляется переход
    \param url       ссылка, ведущая на товарное предложение
    \param server    адрес сервера, выдавшего ссылку
    \param token     токен для проверки действительности перехода
    \param loc	      адрес страницы, на которой показывается информер
    \param ref	      адрес страницы, откуда пришёл посетитель

    Пары (параметр=значение) разделяются символом \c '\\n' (перевод строки).
    Значения никак не экранируются, считается, что они не должны содержать
    символа-разделителя \c '\\n'.
*/
class GenerateRedirectLink
{
    long long informerId;
    string server_ip_;
    string redirect_script_;
    string location_;
public:
    GenerateRedirectLink( long long &informerId,
                         const string &server_ip,
                         const string &redirect_script,
                         const string &location)
        : informerId(informerId), server_ip_(server_ip),
          redirect_script_(redirect_script),
          location_(location) { }

    void operator ()(Offer *offer)
    {
        offer->gen();

        offer->redirect_url = redirect_script_ + "?" + base64_encode(boost::str(
                           boost::format("id=%s\ninf=%d\ntoken=%s\nurl=%s\nserver=%s"
                                         "\nloc=%s")
                           % offer->id_int
                           % informerId
                           % offer->token
                           % offer->url
                           % server_ip_
                           % location_
                       ));
    }
};



/** Обработка запроса на показ рекламы с параметрами ``params``.
	Изменён RealInvest Soft */
std::string Core::Process(const Params &params, vector<Offer*> &items)
{
   // Log::info("[%ld]Core::Process start",tid);
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = microsec_clock::local_time();

    hm->setKey(params.getUserKey());

    informer = getInformer(params);
    //Log::info("[%ld]getInformer done",tid);
    /*
        request_processed_++;
        time_request_started_ = microsec_clock::local_time();

        Log::gdb("start Core");
        startTime = microsec_clock::local_time();
        Informer informer(params.informer_);
        if (!informer.valid())
        {
            Log::warn("Invalid informer, returning void");
            RequestDebugInfo(params);
            return std::string();
        }
        Log::gdb("got informer: %s", to_simple_string(microsec_clock::local_time() - startTime).c_str());
    */
    //Создаем хранилише РП
   bool cleared;

    //получаем список кампаний.
    /*
    list<Campaign> camps;
    list<Campaign> campsSoc;
    list<Campaign> allGeoCamps;
    getCampaigns(params, camps);
    getSocCampaigns(params, campsSoc);
    getAllGeoCampaigns(params, allGeoCamps);
    //получаем вектор идентификаторов допустимых для данного пользователя предложений
    list<pair<pair<string, float>, pair<string, pair<string, string>>>> offersIds;
    */
    /*
        try
        {
            //Запрос к индексу на получение РП
            offersIds = HistoryManager::instance()->getOffersByUser1(params, camps, campsSoc, allGeoCamps);
        }
        catch (std::exception const &ex)
        {
            Log::err("exception %s : %s", typeid(ex).name(), ex.what());
        }*/
    //endTime = microsec_clock::local_time();

    //LOG(INFO) << "время получения РП = " << (endTime - startTime) << "\n";

    ////если полученный вектор пуст, используем старый алгоритм, в противном случае используем наш алгоритм
    getOffers(params, items);
    //Log::gdb("%d offers done",items.size());
    /*
        if (offersIds.size()==0)
        {
            Log::warn("Сработала старая ветка алгоритма");
        //    offers = getOffers(params);
        }
        else
        {*/
    //новый алгоритм
    Log::info("[%ld]get offers: %s %d",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str(), items.size());
    RISAlgorithm(items, params, cleared);
    //Log::info("RISAlgorithm: done",tid);
    //offers = getOffersRIS(offersIds, params, camps, clean, updateShort, updateContext);
    if (!items.size())
    {
        Log::warn("offers empty");
        //offers.assign(informer.capacity(), Offer(""));
    }
    //  }


    // Если нужно показать только социальную рекламу, а настройках стоит
    // опция "В случае отсутствия релевантной рекламы показывать
    // пользовательский код", то возвращаем пользовательскую заглушку
    /*
        if (informer.nonrelevant() == Informer::Show_UserCode && !params.json_)
        {
            bool all_social = true;
            for (auto it = offers.begin(); it != offers.end(); it++)
            {
                if (!Campaign(it->campaign_id()).social())
                {
                    all_social = false;
                    break;
                }
            }
            if (all_social)
                return informer.user_code();
        }
    */

   /// Log::info("[%ld]token_generator transform done",tid);
    // Составляем ссылку перенаправления для каждого элемента

    GenerateRedirectLink redirect_generator(informer->id_int,
                                            server_ip(),
                                            redirect_script(),
                                            params.location_);
    //Log::info("[%ld]GenerateRedirectLink: done",tid);
    std::for_each(items.begin(), items.end(), redirect_generator);
   // Log::info("[%ld]redirect_generator: done size:%d",tid,items.size());
    std::string ret;
    if (params.json_)
        ret = OffersToJson(items);
    else
        ret = OffersToHtml(items, params.getUrl());
    //Log::info("OffersToJson/OffersToHtml: done");
    delete informer;

    //Log::info("[%ld]core time: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());
    return ret;
}

void Core::ProcessSaveResults(const Params &params, const vector<Offer*> &items)
{
/*
    //Задаем значение очистки истории показов
    bool clean = false;
    //Задаем обнавление краткосрочной истории
    bool updateShort = false;
    //Задаём обнавление долгосрочной истории
    bool updateContext = false;
*/
// Сохраняем выданные ссылки в базе данных

    try
    {
        hm->setDeprecatedOffers(items);
//        list<string> shortTerm = HistoryManager::instance()->getShortHistoryByUser(params);
//        list<string> longTerm = HistoryManager::instance()->getLongHistoryByUser(params);
//        list<string> contextTerm = HistoryManager::instance()->getContextHistoryByUser(params);
//        markAsShown(items, params, shortTerm, longTerm, contextTerm);
        //обновление deprecated (по оставшемуся количеству показов) и краткосрочной истории пользователя (по ключевым словам)
//        HistoryManager::instance()->updateUserHistory(items, params, clean, updateShort, updateContext);
    }
    catch (mongo::DBException &ex)
    {
        Log::err("DBException duriAMQPMessageng markAsShown(): %s", ex.what());
    }
    pStmtInformer->Reset();
  //  pStmtOffer->Reset();
}

Informer *Core::getInformer(const Params &params)
{
    pStmtInformer->BindString(1, params.informer_);
    while(pStmtInformer->FetchRow())
    {
        return new Informer(pStmtInformer->GetColumnInt64(0),
                              pStmtInformer->GetColumnInt(1),
                              pStmtInformer->GetColumnString(2),
                              pStmtInformer->GetColumnString(3),
                              pStmtInformer->GetColumnInt64(4),
                              pStmtInformer->GetColumnInt64(5)
                             );
    }
    //pStmtInformer->Reset();
    //pStmtInformer->FreeQuery();
    return nullptr;
}
/**
    Алгоритм работы таков:
file::memory:?cache=shared
	-#  Получаем рекламные кампании, которые нужно показать в этот раз.
	    Выбор кампаний происходит случайно, но с учётом веса. На данный момент
        вес всех кампаний одинаков. Со временем можно сделать составной вес,
        который включал бы в себя цену за клик, CTR и т.д. (расчёт веса
        предварительно производится внешним скриптом).
        .
	-#  Для каждого "места", которое должно быть занято определённой
	    кампанией (п. 1) выбираем товар из этой кампании. Товар также
	    выбирается случайно, но с учётом веса, включающего в себя CTR и др.
	    (также расчитывается внешним скриптом). На данный момент все товары
        равны.
        .
	-#  При малом кол-ве предложений случайное распределение может давать
	    одинаковые элементы в пределах одного информера. Поэтому после
	    получения предложения на п.2, мы проверяем его на уникальность в
	    пределах информера, и если элемент является дубликатом,
	    предпринимаем ещё одну попытку получить предложение. Во избежание
	    вечных циклов, количество попыток ограничивается константой
	    \c Remove_Duplicates_Retries. Увеличение этой константы ведёт к
	    уменьшению вероятности попадания дубликатов, но увеличивает
	    время выполнения. Кроме того, удаление дубликатов меняет
	    весовое распределение предложений. \n
        .
        Иногда удалять дубликаты предложений в пределах кампании недостаточно.
        Например, мы должны показать две кампании. Первая из них содержит всего
        одно предолжение, вторая -- сто. Может случиться, что три места на
        информере будут принадлежать первой кампании. Понятно, что сколько бы
        попыток выбрать "другой" товар этой кампании, у нас ничего не выйдет
        и возникнет дубликат. Поэтому, после \c Remove_Duplicates_Retries
        попыток выбора предложения будет выбрана другая кампания, и цикл
        повториться. Количество возможных сменcookie_id_ кампаний задаётся константой
        \c Change_Campaign_Retries.
        .
	-#  Предложения, указанные в \c params.exluded_offers, по возможности
	    исключаются из просмотра. Это используется в прокрутке информера.
 */
/*
 bool Core::getOffers(const Params &params, std::vector<Offer*> &result)
{
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = microsec_clock::local_time();

//    Log::info("getOffers start");
    std::string depOffers;
    hm->getDeprecatedOffers(depOffers);

    //Log::info("[%ld]get history size: %s %d",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str(), depOffers.size());

    try
    {
        teasersCount = 0;

        pStmtOffer->BindInt64(1, informer->domainId);
        pStmtOffer->BindInt64(2, informer->accountId);
        pStmtOffer->BindInt64(3, informer->id);
        pStmtOffer->BindString(4, params.getCountry());
        pStmtOffer->BindString(5, params.getRegion());
        pStmtOffer->BindString(6, depOffers);

        while(pStmtOffer->FetchRow())
        {
            Offer *off = new Offer(pStmtOffer->GetColumnString(1),
                              pStmtOffer->GetColumnInt64(0),
                              pStmtOffer->GetColumnString(2),
                              pStmtOffer->GetColumnString(3),
                              pStmtOffer->GetColumnString(4),
                              pStmtOffer->GetColumnString(5),
                              pStmtOffer->GetColumnString(6),
                              pStmtOffer->GetColumnString(7),
                              pStmtOffer->GetColumnString(8),
                              true,
                              pStmtOffer->GetColumnBool(9),
                              pStmtOffer->GetColumnInt(10),
                              pStmtOffer->GetColumnDouble(11),
                              pStmtOffer->GetColumnInt(12),
                              pStmtOffer->GetColumnInt(13),
                              pStmtOffer->GetColumnInt(14)
                             );
            off->social = pStmtOffer->GetColumnBool(15);

            if(!off->social)
                all_social = false;

            if(off->type == Offer::Type::teazer) teasersCount++;
            result.push_back(off);
        }
        //pStmtOffer->Reset();
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        return false;
    }
//    Log::info("[%ld]get getoffer done: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());

    return true;
}
*/

bool Core::getOffers(const Params &params, std::vector<Offer*> &result)
{
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = microsec_clock::local_time();

    Kompex::SQLiteStatement *pStmt;

//    Log::info("getOffers start");
    std::string depOffers;
    hm->getDeprecatedOffers(depOffers);

    //Log::info("[%ld]get history size: %s %d",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str(), depOffers.size());

    try
    {
        pStmt = new SQLiteStatement(pDb->pDatabase);

        sqlite3_snprintf(CMD_SIZE, cmd, pStmtOfferStr.c_str(),
                              informer->domainId,
                              informer->accountId,
                              informer->id,
                              params.getCountry().c_str(),
                              params.getRegion().c_str(),
                              depOffers.c_str());
        pStmt->Sql(cmd);
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: pStmtOffer: %s: %s", ex.GetString().c_str(), pDb->getSqlFile("requests/01.sql").c_str());
        delete pStmt;
        return false;
    }
//    Log::info("[%ld]exec: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());
    try
    {
        teasersCount = 0;
        while(pStmt->FetchRow())
        {
            Offer *off = new Offer(pStmt->GetColumnString(1),
                              pStmt->GetColumnInt64(0),
                              pStmt->GetColumnString(2),
                              pStmt->GetColumnString(3),
                              pStmt->GetColumnString(4),
                              pStmt->GetColumnString(5),
                              pStmt->GetColumnString(6),
                              pStmt->GetColumnString(7),
                              pStmt->GetColumnString(8),
                              true,
                              pStmt->GetColumnBool(9),
                              pStmt->GetColumnInt(10),
                              pStmt->GetColumnDouble(11),
                              pStmt->GetColumnInt(12),
                              pStmt->GetColumnInt(13),
                              pStmt->GetColumnInt(14)
                             );
            off->social = pStmt->GetColumnBool(15);

            if(!off->social)
                all_social = false;

            if(off->type == Offer::Type::teazer) teasersCount++;

            result.push_back(off);
        }
        pStmt->FreeQuery();
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        delete pStmt;
        return false;
    }

    delete pStmt;
    //Log::info("[%ld]get getoffer done: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());

    return true;
}

std::string Core::OffersToHtml(const std::vector<Offer*> &items, const std::string &url) const
{
    //Benchmark bench("OffersToHtml закончил свою работу");
    std::string informer_html;

    //для отображения передаётся или один баннер, или вектор тизеров. это и проверяем
    if (!items.empty() && items[0]->isBanner)
    {
        // Получаем HTML-код информера для отображение баннера
        informer_html =
            boost::str(boost::format(InformerTemplate::instance()->getBannersTemplate())
                       % informer->bannersCss
                       % InformerTemplate::instance()->getSwfobjectLibStr()
                       % OffersToJson(items));
    }
    else
    {
        // Получаем HTML-код информера для отображение тизера
        informer_html =
            boost::str(boost::format(InformerTemplate::instance()->getTeasersTemplate())
                       % informer->teasersCss
                       % OffersToJson(items)
                       % informer->capacity
                       % url);
    }

    return informer_html;
}


std::string Core::OffersToJson(const vector<Offer*> &items) const
{

    if(items.empty()) Log::warn("No impressions items to show");

    std::stringstream json;
    json << "[";
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        if (it != items.begin())
            json << ",";

        json << (*it)->toJson();
    }

    json << "]";

    return json.str();
}




/** Возвращает отладочную информацию про текущий запрос */
std::string Core::RequestDebugInfo(const Params &params) const
{
    std::ostringstream out;
    out << "ip: " << params.ip_ << ", "
        "inf: " << params.informer_;
    return out.str();
}


/**Формирование списка РП с учетом их рейтинга для конкретного рекламного блока*/
vector<Offer*> Core::getOffersRIS(const list<pair<pair<string, float>, pair<string,
                                 pair<string, string>>>> &offersIds, const Params &params,
                                 const list<Campaign> &camps, bool &clean, bool &updateShort, bool &updateContext)
{
    vector<Offer*> result;
    createVectorOffersByIds(offersIds, result, camps, params, updateShort, updateContext);
//    RISAlgorithm(result, params, clean);
    return result;
}

/**
 * Создание вектора из РП по списку идентификаторов РП offersIds с учетом рейтингов РП для данного рекламного блока.
 *
 * \param offersIds - список пар (идентификатор, вес), отсортированный по убыванию веса.
 * \param result - результирующий вектор РП.
 * \param camps - список кампаний, по которым выбирался offersIds.
 * \param params - параметры запроса.
 *
 *
 *
 * Возвращает вектор из РП, созданный по списку offersIds. РП в результирующем векторе отсортированы по рейтингу в рамках одного веса.
 *
 * Пример.
 * \code
 * Список offersIds:
 * <a,1.0>
 * <b,1.0>
 * <c,1.0>
 * <e,0.74>
 * <f,0.74>
 * \endcode
 *
 * Соответствующие пары <идентификатор,рейтинг> (привожу парами, чтоб понятней было; рейтинг берётся из структуры OfferData, т.е. тянется из базы):
 * \code
 * <a,7.44>
 * <b,1.5>
 * <c,54.0>
 * <e,123.7>
 * <f,12.0>
 * \endcode
 *
 * Другими словами, у РП a рейтинг 7.44, у РП b - 1.5, с - 54.0. Веса соответствия получили такие: у РП a - 1.0, b - 1.0, c - 1.0, e - 0.74, f - 0.74. Веса вычисляются в результате обращения к индексу с учётом весовых коэффициентов, задаваемых вручную и считываемых модулем при его [модуля] загрузке.
 * Для удобства сделаю запись в тройках <идентификатор,вес,рейтинг>:
 * \code
 * <a, 1.0, 7.44>
 * <b, 1.0, 1.5>
 * <c, 1.0, 54.0>
 * <e, 0.74, 123.7>
 * <f, 0.74, 12.0>
 * \endcode
 * Тогда в результате работы метода получим:
 * \code
 * <c, 1.0, 54.0>
 * <a, 1.0, 7.44>
 * <b, 1.0, 1.5>
 * <e, 0.74, 123.7>
 * <f, 0.74, 12.0>
 * \endcode
 * РП отсортированы по убыванию рейтинга в рамках одного веса.
 *
 * Если список offersIds пуст, то метод в качестве результата выберет все РП из всех РК, находящихся в списке camps, и отсортирует результат по убыванию рейтинга. Поэтому в методе Process если список offersIds пуст, вызывается старая ветка.
 */
void Core::createVectorOffersByIds(const list<pair<pair<string, float>,
                                   pair<string, pair<string, string>>>> &offersIds,
                                   vector<Offer*> &result,
                                   const list<Campaign> &camps,
                                   const Params& params, bool &updateShort, bool &updateContext)
{
    /*
    Informer informer(params.getInformer());
    list<pair<pair<string, float>, pair<string, pair<string, string>>>>::const_iterator p = offersIds.begin();
    Offer *curOffer;
    string branch;
    //LOG(INFO) << "Список выбранных РП:\n";
    while(p!=offersIds.end())
    {
        //добавляем оффер к результату.
        branch = p->second.first;
        curOffer = new Offer();
        curOffer->id = p->first.first;
        curOffer->rating = p->first.second;
        curOffer->setNewBranch(branch);
        curOffer->setNewConformity(p->second.second.first);
        curOffer->setNewMatching(p->second.second.second);
        //проверяем нашли ли чтото по поисковому запросу, стоит ли
        //обнавлять краткосрочную историю
        if (branch == "L2" or branch == "L7" or branch == "L12" or branch == "L17")
        {
            updateShort = true;
        }
        //проверяем нашли ли чтото по контексту, стоит ли
        //обнавлять контекстную историю
        if (branch == "L3" or branch == "L8" or branch == "L13" or branch == "L18")
        {
            updateContext = true;
        }
        //проверка на размер.
        if (curOffer->valid() && checkBannerSize(*curOffer, informer))
        {
            //если подходит по размеру, добавляем.
            result.push_back(*curOffer);
        }
        delete curOffer;
        p++;
    }
    //LOG(INFO) << "после цикла.\n";
    //LOG(INFO) << "result.size()=" << result.size();
    if(result.empty())return;

    if (params.json_)
    {
        vector<Offer>::iterator p;
        p = std::remove_if(result.begin(), result.end(), CExistElementFunctorByType("banner", EOD_TYPE));
        result.erase(p, result.end());
    }
    //LOG(INFO) << "result.size()=" << result.size();
    //LOG(INFO) << "createVectorOffersByIds end";
    */
}


void Core::filterOffersSize(vector<Offer*> &result, const string& informerId)
{
    //LOG(INFO) << "filterOffersSize start\n";
//    filterOffersSize(result, Informer(informerId));
    //LOG(INFO) << "filterOffersSize end\n";
}

void Core::filterOffersSize(vector<Offer*> &result, const Informer& informer)
{
    //LOG(INFO) << "filterOffersSize start\n";
    //размеры баннеров
    vector<Offer*>::iterator p = result.begin();
    while(p != result.end())
    {
        if (checkBannerSize(*p, informer))
        {
            p++;
        }
        else
        {
            p = result.erase(p);
        }

    }
    //LOG(INFO) << "filterOffersSize end\n";
}


/**
 * Проверяет соответствие размера баннера и размера банероместа РБ
 */
bool Core::checkBannerSize(const Offer *offer, const Informer& informer)
{
    if (offer->isBanner)
    {
        if (offer->width != informer.width_banner || offer->height != informer.height_banner)
        {
            return false;
        }
    }
    return true;
}
/**
 * Основной алгоритм.
	1. если первое РП - баннер - выбрать баннер. конец работы алгоритма.
	2. посчитать тизеры:
	    кол-во тизеров < кол-ва мест на РБ -> шаг 12.
	    нет -> шаг 14.
	шаг 12 :
	вычислить средний рейтинг РП типа тизер в последовательности.
	найти первый баннер.
	если баннер есть и его рейтинг > среднего рейтинга по тизерам - отобразить баннер.
	иначе - выбрать все тизеры с дублированием.
	шаг 14 :
	выбрать самый левый тизер.
	его РК занести в список РК
	искать тизер, принадлежащий не к выбранным РК.
	повторяем, пока не просмотрен весь список.
	если выбранных тизеров достаточно для РБ, показываем.
	если нет - добираем из исходного массива стоящие слева тизеры.
 */
void Core::RISAlgorithm(vector<Offer*> &result, const Params &params, bool &clean)
{
    if(result.size() < 5)
        return;

    Offer::it p;

    //Удаляем социалку
    if (!all_social)
    {
        //LOG(INFO) << "Удаляем социалку";
        p = result.begin();
        while(p!=result.end())
        {
            if ((*p)->social)
                result.erase(p);
            else p++;
        }
    }
    else
    {
        //Так как у нас осталась одна социалка
        //заставляем очиститься историю показов пользователей
        //если товаров будет хватать, то происходить это будет редко
        clean = true;
    }
    //если первый элемент баннер, возвращаем баннер.
    if ( result[0]->isBanner && !result[0]->social)
    {
        //NON social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        return;
    }
    if ( result[0]->isBanner && result[0]->social && result.size()==1)
    {
        //social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        return;
    }
    //Первый элемент не баннер
    //посчитать тизеры.
    //int teasersCount = std::count_if(result.begin(),result.end(),CExistElementFunctorByType(Offer::Type::teazer, EOD_TYPE));
    //Log::info("result: %d teasersCount %d informer->capacity: %d", result.size(), teasersCount, informer->capacity);
    //кол-во тизеров < кол-ва мест на РБ -> шаг 12.
    //нет -> шаг 14.

    if (teasersCount <= informer->capacity)
    {
        //LOG(INFO) << "teasersCount <= informer.capacity";
        //шаг 12
        //вычислить средний рейтинг РП типа тизер в последовательности.
        double avgTR = mediumRating(result, Offer::Type::teazer);//"teaser"
        //найти первый баннер
        p = std::find_if(result.begin(),result.end(), OfferExistByType(Offer::Type::banner));
        //если баннер есть и его рейтинг > среднего рейтинга по тизерам - отобразить баннер
        if (p!=result.end() && (*p)->rating > avgTR)
        {
            result.erase(result.begin(), p);
            p++;
            result.erase(p, result.end());
            return;
        }
        //баннер не найден или его рейтинг <= среднего рейтинга тизеров
        //иначе - выбрать все тизеры с дублированием
        //т.е. удалить все баннеры и добавлять в конец вектора существующие элементы
        p = std::remove_if(result.begin(), result.end(), OfferExistByType(Offer::Type::banner));
        result.erase(p, result.end());
        int c=0;
        while ((int)result.size() < informer->capacity)
        {
            if (c>=teasersCount)
            {
                c=0;
            }
            result.push_back(result[c]);
            c++;
        }
        clean = true;
    }
    else//teasersCount > informer->capacity
    {
        //шаг 14
        //удаляем все баннеры, т.к. с ними больше не работаем
        p = std::remove_if(result.begin(), result.end(), OfferExistByType(Offer::Type::banner));
        result.erase(p, result.end());

        //выбрать самый левый тизер.
        //его РК занести в список РК
        //искать тизер, принадлежащий не к выбранным РК.
        //повторяем, пока не просмотрен весь список.
        //если выбранных тизеров достаточно для РБ, показываем.
        //если нет - добираем из исходного массива стоящие слева тизеры.

        list<string> camps;
        vector<Offer*> newResult;

        p = result.begin();
        //LOG(INFO) << "first";
        while(p!=result.end())
        {
            //если кампания тизера не занесена в список, выбираем тизер, выбираем кампанию
            //LOG(INFO) << !isStrInList((*p).campaign_id(), camps) << ((*p).rating() > 0.0);
            if(!isStrInList((*p)->campaign_id, camps) && ((*p)->rating > 0.0))
            {
                //LOG(INFO) << "add";
                newResult.push_back((*p));
                camps.push_back((*p)->campaign_id);
            }
            p++;
        }
        //LOG(INFO) << "first count " << (int)newResult.size() ;
        //если выбрали тизеров меньше, чем мест в информере, добираем тизеры из исходного вектора
        int passage;
        passage = 0;
        while ((passage <  informer->capacity) && ((int)newResult.size() < informer->capacity))
        {
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second +";
            while(p!=result.end() && ((int)newResult.size() < informer->capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(!isOfferInVector((*p), newResult))
                {
                    if(!isStrInList((*p)->campaign_id, camps) && (((*p)->rating > 0.0) || (*p)->branch != "L30") )
                    {
                        //LOG(INFO) << "add";
                        newResult.push_back((*p));
                        camps.push_back((*p)->campaign_id);
                    }
                }
                //LOG(INFO) << "second + count " << (int)newResult.size() ;
                p++;
            }
            passage++;
        }
        //теперь, если без повторений добрать не получилось, дублируем тизеры.
        //LOG(INFO) << "result count " << (int)newResult.size() ;
        p = result.begin();
        while(p!=result.end() && ((int)newResult.size() < informer->capacity))
        {
            //доберём всё за один проход, т.к. result.size > informer.capacity
            //пробуем сначала добрать без повторений.
            if ((*p)->rating > 0.0 || (*p)->branch != "L30" )
            {
                newResult.push_back((*p));
            }
            p++;
        }
        passage = 0;
        while ((passage <  informer->capacity) && ((int)newResult.size() < informer->capacity))
        {
            clean = true;
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second -";
            while(p!=result.end() && ((int)newResult.size() < informer->capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(!isOfferInVector((*p), newResult))
                {
                    if(!isStrInList((*p)->campaign_id, camps) && ((*p)->rating <= 0.0))
                    {
                        //LOG(INFO) << "add";
                        newResult.push_back((*p));
                        camps.push_back((*p)->campaign_id);
                    }
                }
                //LOG(INFO) << "second - count " << (int)newResult.size() ;
                p++;
            }
        }
        p = result.begin();
        while(p!=result.end() && ((int)newResult.size() < informer->capacity))
        {
            //доберём всё за один проход, т.к. result.size > informer.capacity
            //пробуем сначала добрать без повторений.
            if ((*p)->rating <= 0.0)
            {
                newResult.push_back((*p));
            }
            p++;
        }
        result.assign(newResult.begin(), newResult.begin()+informer->capacity);
    }
}


float Core::mediumRating(const vector<Offer*>& vectorOffers, Offer::Type typeOffer)
{
    float summRating=0;
    int countElements = 0;
    vector<Offer>::size_type i;

    for(i=0; i < vectorOffers.size(); i++)
    {
        if(vectorOffers[i]->type == typeOffer)
        {
            summRating += vectorOffers[i]->rating;
            countElements++;
        }
    }

    return summRating/countElements;
}

/** Добавляет в журнал просмотров log.impressions предложения \a items.
    Если показ информера осуществляется в тестовом режиме, запись не происходит.

    Внимание: Используется база данных, зарегистрированная под именем 'log'.
 */
void Core::markAsShown(const vector<Offer*> &items, const Params &params,
                       list<string> &shortTerm, list<string> &longTerm, list<string> &contextTerm )
{
    if (params.test_mode_)
	return;
    mongo::DB db("log");
	//LOG(INFO) << "writing to log...";

    int count = 0;
	list<string>::iterator it;

	mongo::BSONArrayBuilder b1,b2,b3;
	for (it=shortTerm.begin() ; it != shortTerm.end(); ++it )
		b1.append(*it);
	mongo::BSONArray shortTermArray = b1.arr();
	for (it=longTerm.begin() ; it != longTerm.end(); ++it )
		b2.append(*it);
	mongo::BSONArray longTermArray = b2.arr();
	for (it=contextTerm.begin() ; it != contextTerm.end(); ++it )
		b3.append(*it);
	mongo::BSONArray contextTermArray = b3.arr();

    BOOST_FOREACH (const Offer *i, items) {

	std::tm dt_tm;
	dt_tm = boost::posix_time::to_tm(params.time_);
	mongo::Date_t dt( (mktime(&dt_tm)) * 1000LLU);

	mongo::BSONObj keywords = mongo::BSONObjBuilder().
								append("search", params.getSearch()).
								append("context", params.getContext()).
								append("ShortTermHistory", shortTermArray).
								append("longtermhistory", longTermArray).
								append("contexttermhistory", contextTermArray).
								obj();

		string country = country_code_by_addr(params.ip_);
		string region = region_code_by_addr(params.ip_);

	mongo::BSONObj record = mongo::BSONObjBuilder().genOID().
								append("dt", dt).
								append("id", i->id).
								append("id_int", i->id_int).
								append("title", i->title).
								append("inf", params.informer_).
								append("inf_int", informer->id).
								append("ip", params.ip_).
								append("cookie", params.cookie_id_).
								append("social", i->social).
								append("token", i->token).
								append("type", i->type).
								append("isOnClick", i->isOnClick).
								append("campaignId", i->campaign_id).
								append("campaignId_int", i->campaign_id).
								append("campaignTitle", "").
								append("project", "").
								append("country", (country.empty()?"NOT FOUND":country)).
								append("region", (region.empty()?"NOT FOUND":region)).
								append("keywords", keywords).
								append("branch", i->branch).
								append("conformity", i->conformity).
                                append("matching", i->matching).
								obj();

	db.insert("log.impressions", record, true);
	count++;

	offer_processed_ ++;
	if (i->social) social_processed_ ++;
    }
    //LOG_IF(WARNING, count == 0 ) << "No items was added to log.impressions!";
}
