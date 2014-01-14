#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <ctime>
#include <cstdlib>

#include "Config.h"
#include "Log.h"
#include "Core.h"
#include "DB.h"
#include "InformerTemplate.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "utils/base64.h"

#define CMD_SIZE 8192

int Core::request_processed_ = 0;
int Core::offer_processed_ = 0;
int Core::social_processed_ = 0;

Core::Core() :
    redirect_script_("/redirect")
{
    tid = pthread_self();

    cmd = new char[CMD_SIZE];

    pDb = Config::Instance()->pDb;

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
        pStmtInformer = new Kompex::SQLiteStatement(pDb->pDatabase);
        pStmtInformer->Sql(pDb->getSqlFile("requests/02.sql"));
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: pStmtInformer: %s %s", ex.GetString().c_str(),
                 pDb->getSqlFile("requests/02.sql").c_str());
        exit(1);
    }
//+ boost::lexical_cast<std::string>(getpid()) + boost::lexical_cast<std::string>(tid)
    tmpTableName = "tmp" + std::to_string(getpid()) + std::to_string(tid);

    Kompex::SQLiteStatement *p;
    try
    {
        p = new Kompex::SQLiteStatement(pDb->pDatabase);
        sqlite3_snprintf(CMD_SIZE, cmd, "CREATE TABLE IF NOT EXISTS %s(id INT8 NOT NULL);",
                         tmpTableName.c_str());
        p->SqlStatement(cmd);
        sqlite3_snprintf(CMD_SIZE, cmd, "CREATE INDEX IF NOT EXISTS idx_%s_id ON %s(id);",
                         tmpTableName.c_str(), tmpTableName.c_str());
        p->SqlStatement(cmd);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: create tmp table: %s", ex.GetString().c_str());
        exit(1);
    }
    delete p;

    hm = new HistoryManager(tmpTableName);
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
    std::string server_ip_;
    std::string redirect_script_;
    std::string location_;
public:
    GenerateRedirectLink( long long &informerId,
                          const std::string &server_ip,
                          const std::string &redirect_script,
                          const std::string &location)
        : informerId(informerId), server_ip_(server_ip),
          redirect_script_(redirect_script),
          location_(location) { }

    void operator ()(Offer::Pair p)
    {
        p.second->gen();

        p.second->redirect_url = redirect_script_ + "?" + base64_encode(boost::str(
                                     boost::format("id=%s\ninf=%d\ntoken=%s\nurl=%s\nserver=%s\nloc=%s")
                                     % p.second->id_int
                                     % informerId
                                     % p.second->token
                                     % p.second->url
                                     % server_ip_
                                     % location_
                                 ));
    }
};



/** Обработка запроса на показ рекламы с параметрами ``params``.
	Изменён RealInvest Soft */
std::string Core::Process(const Params &params, Offer::Map &items)
{
    // Log::info("[%ld]Core::Process start",tid);
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = boost::posix_time::microsec_clock::local_time();

    //load all history async
    hm->setParams(params);

    informer = getInformer(params);
    //Log::info("[%ld]getInformer done",tid);
    getOffers(params, items);

    //wait all history load
    //hm->waitAsyncHistory();

    //новый алгоритм
    RISAlgorithm(items, params);
    //Log::info("RISAlgorithm: done",tid);
    //offers = getOffersRIS(offersIds, params, camps, clean, updateShort, updateContext);
    if (!items.size())
    {
        Log::warn("offers empty");
        hm->clean = true;
    }

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

    // Составляем ссылку перенаправления для каждого элемента
    GenerateRedirectLink redirect_generator(informer->id_int,
                                            server_ip(),
                                            redirect_script(),
                                            params.location_);

    std::for_each(items.begin(), items.end(), redirect_generator);
    std::string ret;
    if (params.json_)
        ret = OffersToJson(items);
    else
        ret = OffersToHtml(items, params.getUrl());

    delete informer;

    Log::info("[%ld]core time: %s %d",tid, boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time() - startTime).c_str(), items.size());

    return ret;
}

void Core::ProcessSaveResults(const Params &params, const Offer::Map &items)
{
    // Сохраняем выданные ссылки в базе данных
    markAsShown(items, params);

    //обновление deprecated (по оставшемуся количеству показов) и краткосрочной истории пользователя (по ключевым словам)
    hm->updateUserHistory(items, params);

    Kompex::SQLiteStatement *p;
    try
    {
        p = new Kompex::SQLiteStatement(pDb->pDatabase);
        std::string sql("DELETE FROM " + tmpTableName + ";");
        p->SqlStatement(sql);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: delete from %s table: %s", tmpTableName.c_str(), ex.GetString().c_str());
    }
    delete p;

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
    //reset moved to after response function
    //pStmtInformer->Reset();
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

bool Core::getOffers(const Params &params, Offer::Map &result)
{
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = boost::posix_time::microsec_clock::local_time();

    Kompex::SQLiteStatement *pStmt;

//    Log::info("getOffers start");
    hm->getDeprecatedOffersAsyncWait();
    //Log::info("[%ld]get history size: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());

    try
    {
        pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);

        sqlite3_snprintf(CMD_SIZE, cmd, pStmtOfferStr.c_str(),
                         informer->domainId,
                         informer->accountId,
                         informer->id,
                         params.getCountry().c_str(),
                         params.getRegion().c_str(),
                         getpid(),
                         tid);
        pStmt->Sql(cmd);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: pStmtOffer: %s: %s", ex.GetString().c_str(), pDb->getSqlFile("requests/01.sql").c_str());
        delete pStmt;
        return false;
    }
//    Log::info("[%ld]exec: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());
    try
    {
        teasersCount = 0;
        teasersMediumRating = 0;
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
                                   pStmt->GetColumnInt64(8),
                                   true,
                                   pStmt->GetColumnBool(9),
                                   pStmt->GetColumnInt(10),
                                   pStmt->GetColumnDouble(11),
                                   pStmt->GetColumnBool(12),
                                   pStmt->GetColumnInt(13),
                                   pStmt->GetColumnInt(14),
                                   pStmt->GetColumnInt(15)
                                  );
            off->social = pStmt->GetColumnBool(16);

            if(!off->social)
                all_social = false;

            if(off->type == Offer::Type::teazer)
            {
                teasersCount++;
                teasersMediumRating += off->rating;
            }

            result.insert(Offer::Pair(off->id_int,off));
        }
        pStmt->FreeQuery();

        teasersMediumRating /= teasersCount;
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        delete pStmt;
        return false;
    }

    delete pStmt;
    //Log::info("[%ld]get getoffer done: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());
    /*
    if(result.size() < 3 && countDown-- > 0)
    {
        hm->clearDeprecatedOffers();
        Core::getOffers(params, result);
        Log::info("clear view history: %d", countDown);
    }
    */
    return true;
}

std::string Core::OffersToHtml(const Offer::Map &items, const std::string &url) const
{
    std::string informer_html;

    //для отображения передаётся или один баннер, или вектор тизеров. это и проверяем
    if (items.size() > 0 && items.begin()->second->isBanner)
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


std::string Core::OffersToJson(const Offer::Map &items) const
{

    if(!items.size()) Log::warn("No impressions items to show");

    std::stringstream json;
    json << "[";
    for (Offer::cit it = items.begin(); it != items.end(); ++it)
    {
        if (it != items.begin())
            json << ",";

        json << it->second->toJson();
    }

    json << "]";

    return json.str();
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
/*
void Core::createVectorOffersByIds(const list<pair<pair<string, float>,
                                  std::map<long, Offer*> &result,
                                  const Params& params)
{
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
       curOffer->branch = branch;
       curOffer->conformity = p->second.second.first;
       curOffer->matching = p->second.second.second;
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
       if (curOffer->valid && checkBannerSize(curOffer))
       {
           //если подходит по размеру, добавляем.
           result.push_back(curOffer);
       }
       delete curOffer;
       p++;
   }
   //LOG(INFO) << "после цикла.\n";
   //LOG(INFO) << "result.size()=" << result.size();
   if(result.empty())return;

   if (params.json_)
   {
       Offer::it p;
       p = std::remove_if(result.begin(), result.end(), CExistElementFunctorByType("banner", EOD_TYPE));
       result.erase(p, result.end());
   }
   //LOG(INFO) << "result.size()=" << result.size();
   //LOG(INFO) << "createVectorOffersByIds end";
}
*/
/**
 * Проверяет соответствие размера баннера и размера банероместа РБ
 */
bool Core::checkBannerSize(const Offer *offer)
{
    if (offer->isBanner)
    {
        if (offer->width != informer->width_banner || offer->height != informer->height_banner)
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
void Core::RISAlgorithm(Offer::Map &result, const Params &params)
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
            if (p->second->social)
                result.erase(p);
            else p++;
        }
    }
    else
    {
        //Так как у нас осталась одна социалка
        //заставляем очиститься историю показов пользователей
        //если товаров будет хватать, то происходить это будет редко
        hm->clean = true;
    }

    //если первый элемент баннер, возвращаем баннер.
    if(result.begin()->second->isBanner && !result.begin()->second->social)
    {
        //NON social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        return;
    }

    if(result.begin()->second->isBanner && result.begin()->second->social && result.size()==1)
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
//        double avgTR = mediumRating(result, Offer::Type::teazer);//"teaser"
        //найти первый баннер
        p = std::find_if(result.begin(),result.end(), OfferExistByType(Offer::Type::banner));
        //если баннер есть и его рейтинг > среднего рейтинга по тизерам - отобразить баннер
        if (p!=result.end() && p->second->rating > teasersMediumRating)
        {
            result.erase(result.begin(), p);
            p++;
            result.erase(p, result.end());
            return;
        }
        //баннер не найден или его рейтинг <= среднего рейтинга тизеров
        //иначе - выбрать все тизеры с дублированием
        //т.е. удалить все баннеры и добавлять в конец вектора существующие элементы
        for( Offer::it o = result.begin(); o != result.end(); )
        {
            if( o->second->type == Offer::Type::banner)
            {
                o = result.erase(o);
            }
            else
            {
                ++o;
            }
        }

        int c=0;
        while ((int)result.size() < informer->capacity)
        {
            if(c>=teasersCount)
            {
                c=0;
            }

            result.insert(Offer::Pair(c,result.begin()->second));
            c++;
        }
        hm->clean = true;
    }
    else//teasersCount > informer->capacity
    {
        //шаг 14
        //удаляем все баннеры, т.к. с ними больше не работаем
        for( Offer::it o = result.begin(); o != result.end(); )
        {
            if( o->second->type == Offer::Type::banner)
            {
                o = result.erase(o);
            }
            else
            {
                ++o;
            }
        }
        //выбрать самый левый тизер.
        //его РК занести в список РК
        //искать тизер, принадлежащий не к выбранным РК.
        //повторяем, пока не просмотрен весь список.
        //если выбранных тизеров достаточно для РБ, показываем.
        //если нет - добираем из исходного массива стоящие слева тизеры.

        std::map<const long,long> camps;
        Offer::Map newResult;

        p = result.begin();
        //LOG(INFO) << "first";
        while(p!=result.end())
        {
            //если кампания тизера не занесена в список, выбираем тизер, выбираем кампанию
            //LOG(INFO) << !isStrInList((*p).campaign_id(), camps) << ((*p).rating() > 0.0);
            if(!camps.count(p->second->campaign_id) && (p->second->rating > 0.0))
            {
                //LOG(INFO) << "add";
                newResult.insert(Offer::Pair(p->first, p->second));
                camps.insert(std::pair<const long, long>(p->second->campaign_id,p->second->campaign_id));
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
                if(newResult.count(p->first))
                {
                    if(!camps.count(p->second->campaign_id) && (
                                (p->second->rating > 0.0) || p->second->branch != "L30") )
                    {
                        //LOG(INFO) << "add";
                        newResult.insert(Offer::Pair(p->first,p->second));
                        camps.insert(std::pair<const long, long>(p->second->campaign_id,p->second->campaign_id));
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
            if (p->second->rating > 0.0 || p->second->branch != "L30" )
            {
                newResult.insert(Offer::Pair(p->first,p->second));
            }
            p++;
        }
        passage = 0;
        while ((passage <  informer->capacity) && ((int)newResult.size() < informer->capacity))
        {
            hm->clean = true;
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second -";
            while(p!=result.end() && ((int)newResult.size() < informer->capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(newResult.count(p->first))
                {
                    if(!camps.count(p->second->campaign_id) && (p->second->rating <= 0.0))
                    {
                        //LOG(INFO) << "add";
                        newResult.insert(Offer::Pair(p->first,p->second));
                        camps.insert(std::pair<const long, long>(p->second->campaign_id,p->second->campaign_id));
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
            if (p->second->rating <= 0.0)
            {
                newResult.insert(Offer::Pair(p->first,p->second));
            }
            p++;
        }
        int i;
        result.clear();
        for(i = 0, p = newResult.begin(); i < informer->capacity; i++, p++)
        {
            result.insert(Offer::Pair(i,p->second));
        }
    }
}

/** Добавляет в журнал просмотров log.impressions предложения \a items.
    Если показ информера осуществляется в тестовом режиме, запись не происходит.

    Внимание: Используется база данных, зарегистрированная под именем 'log'.
 */
void Core::markAsShown(const Offer::Map &items, const Params &params)
{
    if (params.test_mode_)
        return;

    try
    {
        mongo::DB db("log");
        //LOG(INFO) << "writing to log...";

        int count = 0;
        std::list<std::string>::iterator it;

        mongo::BSONArrayBuilder b1,b2,b3;
        for (it=hm->vshortTerm.begin() ; it != hm->vshortTerm.end(); ++it )
            b1.append(*it);
        mongo::BSONArray shortTermArray = b1.arr();
        for (it=hm->vlongTerm.begin() ; it != hm->vlongTerm.end(); ++it )
            b2.append(*it);
        mongo::BSONArray longTermArray = b2.arr();
        for (it=hm->vcontextTerm.begin() ; it != hm->vcontextTerm.end(); ++it )
            b3.append(*it);
        mongo::BSONArray contextTermArray = b3.arr();

        for(Offer::cit i = items.begin(); i != items.end(); ++i)
        {

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

            mongo::BSONObj record = mongo::BSONObjBuilder().genOID().
                                    append("dt", dt).
                                    append("id", i->second->id).
                                    append("id_int", i->second->id_int).
                                    append("title", i->second->title).
                                    append("inf", params.informer_).
                                    append("inf_int", informer->id).
                                    append("ip", params.ip_).
                                    append("cookie", params.cookie_id_).
                                    append("social", i->second->social).
                                    append("token", i->second->token).
                                    append("type", i->second->type).
                                    append("isOnClick", i->second->isOnClick).
                                    //append("campaignId", i->second->campaign_id).
                                    append("campaignId_int", i->second->campaign_id).
                                    append("campaignTitle", "").
                                    append("project", "").
                                    append("country", (params.getCountry().empty()?"NOT FOUND":params.getCountry().c_str())).
                                    append("region", (params.getRegion().empty()?"NOT FOUND":params.getRegion().c_str())).
                                    append("keywords", keywords).
                                    append("branch", i->second->branch).
                                    append("conformity", i->second->conformity).
                                    append("matching", i->second->matching).
                                    obj();

            db.insert("log.impressions", record, true);
            count++;

            offer_processed_ ++;
            if (i->second->social) social_processed_ ++;
        }
    }
    catch (mongo::DBException &ex)
    {
        Log::err("DBException duriAMQPMessageng markAsShown(): %s", ex.what());
    }

    //LOG_IF(WARNING, count == 0 ) << "No items was added to log.impressions!";
}
