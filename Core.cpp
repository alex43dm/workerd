#include "DB.h"
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <mongo/util/version.h>
#include <mongo/bson/bsonobjbuilder.h>

#include "Log.h"
#include "Core.h"
#include "InformerTemplate.h"
#include "utils/base64.h"
#include "utils/benchmark.h"
#include "HistoryManager.h"
#include "utils/Comparators.h"
#include "utils/GeoIPTools.h"
#include <mongo/bson/bsonobjbuilder.h>

#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

using std::list;
using std::vector;
using std::map;
using std::unique_ptr;
using std::string;
using namespace boost::posix_time;
using namespace Kompex;
#define foreach	BOOST_FOREACH

int Core::request_processed_ = 0;
int Core::offer_processed_ = 0;
int Core::social_processed_ = 0;

Core::Core(DataBase *_pDb) :
    redirect_script_("/redirect"),
    pDb(_pDb)
{
    tid = pthread_self();
    std::string sql;

    try
    {
        pStmtInformer = new SQLiteStatement(pDb->pDatabase);
        pStmtInformer->Sql("SELECT id,capacity,bannersCss,teasersCss FROM Informer WHERE guid=@q LIMIT 1");

        pStmtOffer = new SQLiteStatement(pDb->pDatabase);
        pDb->getSqlFile("requests/01.sql",sql);
        pStmtOffer->Sql(sql);
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: %s: %s", ex.GetString().c_str(), sql.c_str());
        exit(1);
    }

    Log::info("[%ld]core start",tid);
}

Core::~Core()
{
    pStmtInformer->FreeQuery();
    delete pStmtInformer;
    pStmtOffer->FreeQuery();
    delete pStmtOffer;
    delete pDb;
}

/** Функтор генерирует токен для предложения ``offer``.
    Возвращает структуру Core::ImpressionItem.

    Токен представляет собой некое уникальное значение, введенное для
    избежания накруток (см. документацию).

    В тестовом режиме токеном является строка "test".
*/
class GenerateToken
{
    Params params_;
public:
    GenerateToken(const Params &params) : params_(params) { }
    Core::ImpressionItem operator ()(const Offer &offer)
    {
        Core::ImpressionItem result(offer);

        // Генерируем токен
        if (params_.test_mode_)
            result.token = "test";
        else
        {
            std::ostringstream s;
            s << std::hex << rand(); // Cлучайное число в шестнадцатиричном
            result.token = s.str();  // исчислении
        }
        return result;
    }
};


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
    string informerId;
    string server_ip_;
    string redirect_script_;
    string location_;
public:
    GenerateRedirectLink(const string &informerId,
                         const string &server_ip,
                         const string &redirect_script,
                         const string &location)
        : informerId(informerId), server_ip_(server_ip),
          redirect_script_(redirect_script),
          location_(location) { }

    void operator ()(Core::ImpressionItem &item)
    {
        string query = boost::str(
                           boost::format("id=%s\ninf=%s\ntoken=%s\nurl=%s\nserver=%s"
                                         "\nloc=%s")
                           % item.offer.id
                           % informerId
                           % item.token
                           % item.offer.url
                           % server_ip_
                           % location_
                       );
        item.redirect_url = redirect_script_ + "?" + base64_encode(query);
    }
};



/** Обработка запроса на показ рекламы с параметрами ``params``.
	Изменён RealInvest Soft */
std::string Core::Process(const Params &params, vector<ImpressionItem> &items)
{
   // Log::info("[%ld]Core::Process start",tid);
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = microsec_clock::local_time();

    Informer *informer = getInformer(params);
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
    vector<Offer> offers;
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
    offers = getOffers(params, *informer);
    //Log::info("[%ld]get %d offers done",tid,offers.size());
    /*
        if (offersIds.size()==0)
        {
            Log::warn("Сработала старая ветка алгоритма");
        //    offers = getOffers(params);
        }
        else
        {*/
    //новый алгоритм
    RISAlgorithm(offers, params, cleared, informer->capacity);
    //Log::info("RISAlgorithm: done",tid);
    //offers = getOffersRIS(offersIds, params, camps, clean, updateShort, updateContext);
    if (!offers.size())
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
    // Каждому элементу просмотра присваиваем уникальный токен
    GenerateToken token_generator(params);
    std::transform(offers.begin(), offers.end(), back_inserter(items), token_generator);
   /// Log::info("[%ld]token_generator transform done",tid);
    // Составляем ссылку перенаправления для каждого элемента

    GenerateRedirectLink redirect_generator(params.informer_,
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
        ret = OffersToHtml(items, params, informer);
    //Log::info("OffersToJson/OffersToHtml: done");
    delete informer;

    Log::info("[%ld]core time: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());
    return ret;
}

void Core::ProcessSaveResults(const Params &params, const vector<ImpressionItem> &items)
{

    //Задаем значение очистки истории показов
    bool clean = false;
    //Задаем обнавление краткосрочной истории
    bool updateShort = false;
    //Задаём обнавление долгосрочной истории
    bool updateContext = false;

// Сохраняем выданные ссылки в базе данных

    try
    {
        list<string> shortTerm = HistoryManager::instance()->getShortHistoryByUser(params);
        list<string> longTerm = HistoryManager::instance()->getLongHistoryByUser(params);
        list<string> contextTerm = HistoryManager::instance()->getContextHistoryByUser(params);
//        markAsShown(items, params, shortTerm, longTerm, contextTerm);
        //обновление deprecated (по оставшемуся количеству показов) и краткосрочной истории пользователя (по ключевым словам)
        HistoryManager::instance()->updateUserHistory(items, params, clean, updateShort, updateContext);
    }
    catch (mongo::DBException &ex)
    {
        Log::err("DBException duriAMQPMessageng markAsShown(): %s", ex.what());
    }

}

Informer *Core::getInformer(const Params &params)
{
    Informer *result;
    pStmtInformer->BindString(1, params.informer_);
    while(pStmtInformer->FetchRow())
    {
        result = new Informer(pStmtInformer->GetColumnInt64(0),
                              pStmtInformer->GetColumnInt(1),
                              pStmtInformer->GetColumnString(2),
                              pStmtInformer->GetColumnString(3)
                             );
    }
    pStmtInformer->Reset();
    //pStmtInformer->FreeQuery();
    return result;
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
vector<Offer> Core::getOffers(const Params &params, const Informer& inf)
{
//    Log::info("getOffers start");
    vector<Offer> result;
    try
    {
        pStmtOffer->BindString(1, "19,32");//from informer domains id
        pStmtOffer->BindInt(2, inf.id);//from informer account id
        pStmtOffer->BindString(3, "UA");//from informer country id

        //pStmtOffer->BindString(2, "UA");//params.location_);
        //pStmtOffer->BindString(2, params.informer_);
    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        exit(1);
    }
    //pStmtInformer->BindString(1, country_code_by_addr("93.77.122.93"));
    //Log::gdb("getOffers start FetchRow");
    while(pStmtOffer->FetchRow())
    {
        Offer off = Offer(pStmtOffer->GetColumnString(1),
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
                          pStmtOffer->GetColumnString(10),
                          pStmtOffer->GetColumnDouble(11),
                          pStmtOffer->GetColumnInt(12),
                          pStmtOffer->GetColumnInt(13),
                          pStmtOffer->GetColumnInt(14)
                         );
        off.social = pStmtOffer->GetColumnBool(15);
        result.push_back(off);
    }
    pStmtOffer->Reset();
    //pStmt->FreeQuery();
//    Log::info("getOffers end");
    return result;
}


std::string Core::OffersToHtml(const std::vector<ImpressionItem> &items, const Params &params, Informer *informer) const
{
    //Benchmark bench("OffersToHtml закончил свою работу");

    stringstream url;
    url << params.script_name_ <<
        "?scr=" << params.informer_ <<
        "&show=json";
    if (params.test_mode_)
        url << "&test=true";
    if (!params.country_.empty())
        url << "&country=" << params.country_;
    if (!params.region_.empty())
        url << "&region=" << params.region_;
    url << "&";

    std::string informer_html;

    //для отображения передаётся или один баннер, или вектор тизеров. это и проверяем
    if (!items.empty() && items[0].offer.isBanner)
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
                       % url.str());
    }

    return informer_html;
}


std::string Core::OffersToJson(const vector<ImpressionItem> &items) const
{
    if(items.empty()) Log::warn("No impressions items to show");
    std::stringstream json;
    json << "[";
    for (auto it = items.begin(); it != items.end(); it++)
    {
        if (it != items.begin())
            json << ",";
        json << "{" <<
             "\"id\": \"" << EscapeJson(it->offer.id) << "\"," <<
             "\"title\": \"" << EscapeJson(it->offer.title) << "\"," <<
             "\"description\": \"" << EscapeJson(it->offer.description) << "\"," <<
             "\"price\": \"" << EscapeJson(it->offer.price) << "\"," <<
             "\"image\": \"" << EscapeJson(it->offer.image_url) << "\"," <<
             "\"swf\": \"" << EscapeJson(it->offer.swf) << "\"," <<
             "\"url\": \"" << EscapeJson(it->redirect_url) << "\"," <<
             "\"token\": \"" << EscapeJson(it->token) << "\"," <<
             "\"rating\": \"" << it->offer.rating << "\"," <<
             "\"width\": \"" << it->offer.width << "\"," <<
             "\"height\": \"" << it->offer.height << "\"" <<
             "}";
    }

    json << "]";

    return json.str();
}


std::string Core::EscapeJson(const std::string &str)
{
    std::string result;
    for (auto it = str.begin(); it != str.end(); it++)
    {
        switch (*it)
        {
        case '\t':
            result.append("\\t");
            break;
        case '"':
            result.append("\\\"");
            break;
        case '\\':
            result.append(" ");
            break;
        case '\'':
            result.append("\\'");
            break;
        case '/':
            result.append("\\/");
            break;
        case '\b':
            result.append("\\b");
            break;
        case '\r':
            result.append("\\r");
            break;
        case '\n':
            result.append("\\n");
            break;
        default:
            result.append(it, it + 1);
            break;
        }
    }
    return result;
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
vector<Offer> Core::getOffersRIS(const list<pair<pair<string, float>, pair<string, pair<string, string>>>> &offersIds, const Params &params, const list<Campaign> &camps, bool &clean, bool &updateShort, bool &updateContext)
{
    vector<Offer> result;
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
                                   vector<Offer> &result,
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


void Core::filterOffersSize(vector<Offer> &result, const string& informerId)
{
    //LOG(INFO) << "filterOffersSize start\n";
//    filterOffersSize(result, Informer(informerId));
    //LOG(INFO) << "filterOffersSize end\n";
}

void Core::filterOffersSize(vector<Offer> &result, const Informer& informer)
{
    //LOG(INFO) << "filterOffersSize start\n";
    //размеры баннеров
    vector<Offer>::iterator p = result.begin();
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


bool Core::isOfferInCampaigns(const Offer& offer, const list<Campaign>& camps)
{
    list<Campaign>::const_iterator p = camps.begin();
    while (p != camps.end())
    {
        if (offer.campaign_id==p->id())
        {
            return true;
        }
        p++;
    }
    return false;
}


/**
 * Проверяет соответствие размера баннера и размера банероместа РБ
 */
bool Core::checkBannerSize(const Offer& offer, const Informer& informer)
{
    if (offer.isBanner)
    {
        if (offer.width != informer.width_banner || offer.height != informer.height_banner)
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
void Core::RISAlgorithm(vector<Offer> &result, const Params &params, bool &clean, int capacity)
{
    if(!result.size())
        return;

    vector<Offer>::iterator p;
    p = result.begin();

    bool all_social = true;
    while(p!=result.end())
    {
        if(!(*p).social)
        {
            all_social = false;
            break;
        }
        p++;
    }
    //Удаляем социалку
    if (!all_social)
    {
        //LOG(INFO) << "Удаляем социалку";
        p = result.begin();
        while(p!=result.end())
        {
            if ((*p).social)
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
    if ( result[0].isBanner && !result[0].social)
    {
        //NON social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        return;
    }
    if ( result[0].isBanner && result[0].social && result.size()==1)
    {
        //social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        return;
    }
    //Первый элемент не баннер
    //посчитать тизеры.
    int teasersCount = std::count_if(result.begin(),result.end(),CExistElementFunctorByType("teaser", EOD_TYPE));
    //LOG(INFO) << "teasersCount " << teasersCount;
    //кол-во тизеров < кол-ва мест на РБ -> шаг 12.
    //нет -> шаг 14.

    if (teasersCount <= capacity)
    {
        //LOG(INFO) << "teasersCount <= informer.capacity";
        //шаг 12
        //вычислить средний рейтинг РП типа тизер в последовательности.
        double avgTR = mediumRating(result, "teaser");
        //найти первый баннер
        p = std::find_if(result.begin(),result.end(), CExistElementFunctorByType("banner", EOD_TYPE));
        //если баннер есть и его рейтинг > среднего рейтинга по тизерам - отобразить баннер
        if (p!=result.end() && (*p).rating > avgTR)
        {
            result.erase(result.begin(), p);
            p++;
            result.erase(p, result.end());
            return;
        }
        //баннер не найден или его рейтинг <= среднего рейтинга тизеров
        //иначе - выбрать все тизеры с дублированием
        //т.е. удалить все баннеры и добавлять в конец вектора существующие элементы
        p = std::remove_if(result.begin(), result.end(), CExistElementFunctorByType("banner", EOD_TYPE));
        result.erase(p, result.end());
        int c=0;
        while ((int)result.size() < capacity)
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
    else
    {
        //шаг 14
        //удаляем все баннеры, т.к. с ними больше не работаем
        p = std::remove_if(result.begin(), result.end(), CExistElementFunctorByType("banner", EOD_TYPE));
        result.erase(p, result.end());

        //выбрать самый левый тизер.
        //его РК занести в список РК
        //искать тизер, принадлежащий не к выбранным РК.
        //повторяем, пока не просмотрен весь список.
        //если выбранных тизеров достаточно для РБ, показываем.
        //если нет - добираем из исходного массива стоящие слева тизеры.

        list<string> camps;
        vector<Offer> newResult;

        p = result.begin();
        //LOG(INFO) << "first";
        while(p!=result.end())
        {
            //если кампания тизера не занесена в список, выбираем тизер, выбираем кампанию
            //LOG(INFO) << !isStrInList((*p).campaign_id(), camps) << ((*p).rating() > 0.0);
            if(!isStrInList((*p).campaign_id, camps) && ((*p).rating > 0.0))
            {
                //LOG(INFO) << "add";
                newResult.push_back((*p));
                camps.push_back((*p).campaign_id);
            }
            p++;
        }
        //LOG(INFO) << "first count " << (int)newResult.size() ;
        //если выбрали тизеров меньше, чем мест в информере, добираем тизеры из исходного вектора
        int passage;
        passage = 0;
        while ((passage <  capacity) && ((int)newResult.size() < capacity))
        {
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second +";
            while(p!=result.end() && ((int)newResult.size() < capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(!isOfferInVector((*p), newResult))
                {
                    if(!isStrInList((*p).campaign_id, camps) && (((*p).rating > 0.0) || (*p).branch != "L30") )
                    {
                        //LOG(INFO) << "add";
                        newResult.push_back((*p));
                        camps.push_back((*p).campaign_id);
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
        while(p!=result.end() && ((int)newResult.size() < capacity))
        {
            //доберём всё за один проход, т.к. result.size > informer.capacity
            //пробуем сначала добрать без повторений.
            if ((*p).rating > 0.0 || (*p).branch != "L30" )
            {
                newResult.push_back((*p));
            }
            p++;
        }
        passage = 0;
        while ((passage <  capacity) && ((int)newResult.size() < capacity))
        {
            clean = true;
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second -";
            while(p!=result.end() && ((int)newResult.size() < capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(!isOfferInVector((*p), newResult))
                {
                    if(!isStrInList((*p).campaign_id, camps) && ((*p).rating <= 0.0))
                    {
                        //LOG(INFO) << "add";
                        newResult.push_back((*p));
                        camps.push_back((*p).campaign_id);
                    }
                }
                //LOG(INFO) << "second - count " << (int)newResult.size() ;
                p++;
            }
        }
        p = result.begin();
        while(p!=result.end() && ((int)newResult.size() < capacity))
        {
            //доберём всё за один проход, т.к. result.size > informer.capacity
            //пробуем сначала добрать без повторений.
            if ((*p).rating <= 0.0)
            {
                newResult.push_back((*p));
            }
            p++;
        }
        result.assign(newResult.begin(), newResult.begin()+capacity);
    }

}


float Core::mediumRating(const vector<Offer>& vectorOffers, const string &typeOfferStr)
{
    float summRating=0;
    int countElements = 0;
    vector<Offer>::size_type i;

    for(i=0; i < vectorOffers.size(); i++)
    {
        if(vectorOffers[i].type == typeOfferStr)
        {
            summRating += vectorOffers[i].rating;
            countElements++;
        }
    }

    return summRating/countElements;
}

bool Core::isSocial (Offer& i)
{
    return Campaign(i.campaign_id).social();
}
