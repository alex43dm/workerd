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
#include "EBranch.h"

#define CMD_SIZE 8192

int HistoryManager::request_processed_ = 0;
int HistoryManager::offer_processed_ = 0;
int HistoryManager::social_processed_ = 0;

Core::Core()
//    redirect_script_("/redirect")
{
    tid = pthread_self();

    cmd = new char[CMD_SIZE];

    pDb = Config::Instance()->pDb;

    pStmtOfferStr = pDb->getSqlFile("requests/01.sql");
    InformerStr  = pDb->getSqlFile("requests/04.sql");
    RetargetingOfferStr = pDb->getSqlFile("requests/03.sql");
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
    std::string location_;
public:
    GenerateRedirectLink( long long &informerId,
                          const std::string &location)
        : informerId(informerId),
          location_(location) { }

    void operator ()(Offer *p)
    {
        p->redirect_url =
        Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                                     boost::format("id=%s\ninf=%d\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                                     % p->id_int
                                     % informerId
                                     % rand()
                                     % p->url
                                     % Config::Instance()->server_ip_
                                     % location_
                                 ));
    }
};



/** Обработка запроса на показ рекламы с параметрами ``params``.
	Изменён RealInvest Soft */
std::string Core::Process(const Params *prms)
{
    unsigned RetargetingCount;
    Offer::Vector vRIS;

    Log::info("[%ld]Core::Process start",tid);
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = boost::posix_time::microsec_clock::local_time();

    params = prms;

    getInformer();
    Log::info("[%ld]getInformer: done",tid);

    //load all history async
    hm->getUserHistory(params);

    RetargetingCount = (int)informer->capacity * Config::Instance()->retargeting_by_persents_ / 100;

    getAllRetargeting(resultRetargeting);
    Log::info("[%ld]getAllRetargeting: done",tid);

    getAllOffers(items);
    Log::info("[%ld]getOffers: done",tid);

    //wait all history load
    hm->sphinxProcess(items, result);
    Log::info("[%ld]sphinxProcess: done",tid);

    //новый алгоритм
    RISAlgorithm(result, vRIS, informer->capacity);
    Log::info("[%ld]RISAlgorithm: vRIS %ld done",tid, vRIS.size());

    RISAlgorithm(resultRetargeting, vOutPut, RetargetingCount);
    Log::info("[%ld]RISAlgorithm: vOutPut %ld done",tid, vOutPut.size());
    //merge
    Offer::itV last;
    if( informer->capacity - vOutPut.size() < vRIS.size())
    {
        last = vRIS.begin() + (informer->capacity - vOutPut.size());
    }
    else
    {
        last = vRIS.end();
    }

    vOutPut.insert(vOutPut.end(),
                   vRIS.begin(),
                   last);

    if (!vOutPut.size())
    {
        Log::warn("offers empty");
        hm->clean = true;
    }
  // hm->getRetargetingAsyncWait();

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

    // Составляем ссылку перенаправления для каждого элемента moved to RIS
    //GenerateRedirectLink redirect_generator(informer->id_int, params.location_);
    //std::for_each(RISResult.begin(), RISResult.end(), redirect_generator);

    std::string ret;
    if (params->json_)
        ret = OffersToJson(vOutPut);
    else
        ret = OffersToHtml(vOutPut, params->getUrl());

    delete informer;

    Log::info("[%ld]core time: %s %d",tid, boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time() - startTime).c_str(), vOutPut.size());

    return ret;
}

void Core::ProcessSaveResults()
{
    if (params->test_mode_)
        return;
    // Сохраняем выданные ссылки в базе данных
    //обновление deprecated (по оставшемуся количеству показов) и краткосрочной истории пользователя (по ключевым словам)
    hm->updateUserHistory(vOutPut, params, informer);

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

    result.clear();
    resultRetargeting.clear();

    vOutPut.clear();

    for (auto o = items.begin(); o != items.end(); ++o)
    {
        if(o->second)
            delete o->second;
        items.erase(o);
    }
    //items.clear();
}

Informer *Core::getInformer()
{
    Kompex::SQLiteStatement *pStmt;

    sqlite3_snprintf(CMD_SIZE, cmd, InformerStr.c_str(), params->informer_.c_str());
    try
    {
        pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);
        pStmt->Sql(cmd);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: getOffers: %s: %s", ex.GetString().c_str(), cmd);
        delete pStmt;
        return nullptr;
    }

    try
    {
        while(pStmt->FetchRow())
        {
            informer =  new Informer(pStmt->GetColumnInt64(0),
                                pStmt->GetColumnInt(1),
                                pStmt->GetColumnString(2),
                                pStmt->GetColumnString(3),
                                pStmt->GetColumnInt64(4),
                                pStmt->GetColumnInt64(5)
                               );
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        delete pStmt;
        return nullptr;
    }

    delete pStmt;

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

bool Core::getAllOffers(Offer::Map &ret)
{
    sqlite3_snprintf(CMD_SIZE, cmd, pStmtOfferStr.c_str(),
                         informer->domainId,
                         informer->accountId,
                         informer->id,
                         params->getCountry().c_str(),
                         params->getRegion().c_str(),
                         getpid(),
                         tid);

    hm->getDeprecatedOffersAsyncWait();
    return getOffers(ret);
}

bool Core::getAllRetargeting(Offer::Vector &ret)
{
    Offer::Map itemsRetargeting;

    std::string ids = hm->getRetargetingAsyncWait();

    sqlite3_snprintf(CMD_SIZE, cmd, RetargetingOfferStr.c_str(), ids.c_str());

    getOffers(itemsRetargeting);

    for(auto i = itemsRetargeting.begin(); i != itemsRetargeting.end(); ++i)
    {
        ret.push_back((*i).second);
    }

    return true;
}

bool Core::getOffers(Offer::Map &result)
{
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = boost::posix_time::microsec_clock::local_time();

    Kompex::SQLiteStatement *pStmt;

//    Log::info("getOffers start");
    //Log::info("[%ld]get history size: %s",tid, to_simple_string(microsec_clock::local_time() - startTime).c_str());

    try
    {
        pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);
        pStmt->Sql(cmd);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: getOffers: %s: %s", ex.GetString().c_str(), cmd);
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

            result.insert(Offer::Pair(off->id_int,off));
        }
        pStmt->FreeQuery();
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        delete pStmt;
        return false;
    }

    delete pStmt;

    return true;
}

std::string Core::OffersToHtml(const Offer::Vector &items, const std::string &url) const
{
    std::string informer_html;

    //для отображения передаётся или один баннер, или вектор тизеров. это и проверяем
    if (items.size() > 0 && (*items.begin())->isBanner)
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


std::string Core::OffersToJson(const Offer::Vector &items) const
{

    if(!items.size()) Log::warn("No impressions items to show");

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
void Core::RISAlgorithm(Offer::Vector &result, Offer::Vector &RISResult, unsigned outLen)
{
    std::map<const long,long> camps;
    Offer::itV p;

    RISResult.clear();

    if(result.size() < 5)
    {
        Log::warn("result size less then 5, return");
        for(auto p = result.begin(); p != result.end() && RISResult.size() < outLen; ++p)
            RISResult.push_back(*p);
        goto make_return;
    }

        for(auto i = result.begin(); i != result.end(); ++i)
            if((*i)->type == Offer::Type::teazer)
            {
                teasersCount++;
                teasersMediumRating += (*i)->rating;
            }
        teasersMediumRating /= teasersCount;


    //Удаляем социалку
    if (!all_social)
    {
        //LOG(INFO) << "Удаляем социалку";
        p = result.begin();
        while(p != result.end())
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
        hm->clean = true;
    }

    //если первый элемент баннер, возвращаем баннер.
    if((*result.begin())->isBanner && !(*result.begin())->social)
    {
        //NON social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        for(auto p = result.begin(); p != result.end() && RISResult.size() < outLen; ++p)
            RISResult.push_back(*p);
        goto make_return;
    }

    if((*result.begin())->isBanner && (*result.begin())->social && result.size()==1)
    {
        //social banner
        p = result.begin();
        p++;
        result.erase(p, result.end());
        for(auto p = result.begin(); p != result.end() && RISResult.size() < outLen; ++p)
            RISResult.push_back(*p);
        goto make_return;
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

        //найти первый баннер
        p = std::find_if(result.begin(),result.end(), OfferExistByType(Offer::Type::banner));
        //если баннер есть и его рейтинг > среднего рейтинга по тизерам - отобразить баннер
        if (p!=result.end() && (*p)->rating > teasersMediumRating)
        {
            result.erase(result.begin(), p);
            p++;
            result.erase(p, result.end());
            for(auto p = result.begin(); p != result.end() && RISResult.size() < outLen; ++p)
                RISResult.push_back(*p);
            goto make_return;
        }
        //баннер не найден или его рейтинг <= среднего рейтинга тизеров
        //иначе - выбрать все тизеры с дублированием
        //т.е. удалить все баннеры и добавлять в конец вектора существующие элементы
        for( auto o = result.begin(); o != result.end(); )
        {
            if( (*o)->type == Offer::Type::banner)
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

            result.push_back(*result.begin());
            c++;
        }
        hm->clean = true;
    }
    else//teasersCount > informer->capacity
    {
        //шаг 14
        //удаляем все баннеры, т.к. с ними больше не работаем
        for( auto o = result.begin(); o != result.end(); )
        {
            if( (*o)->type == Offer::Type::banner)
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

        p = result.begin();
        //LOG(INFO) << "first";
        while(p!=result.end())
        {
            //если кампания тизера не занесена в список, выбираем тизер, выбираем кампанию
            //LOG(INFO) << !isStrInList((*p).campaign_id(), camps) << ((*p).rating() > 0.0);
            if(!camps.count((*p)->campaign_id) && ((*p)->rating > 0.0))
            {
                //LOG(INFO) << "add";
                if(RISResult.size() < outLen) RISResult.push_back(*p); else goto make_return;
                camps.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
            }
            p++;
        }
        //LOG(INFO) << "first count " << (int)RISResult.size() ;
        //если выбрали тизеров меньше, чем мест в информере, добираем тизеры из исходного вектора
        int passage;
        passage = 0;
        while ((passage <  informer->capacity) && ((int)RISResult.size() < informer->capacity))
        {
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second +";
            while(p!=result.end() && ((int)RISResult.size() < informer->capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(std::find(RISResult.begin(), RISResult.end(), *p) != RISResult.end())
                {
                    if(!camps.count((*p)->campaign_id) && (
                                ((*p)->rating > 0.0) || (*p)->branch != EBranchL::L30) )//???never assign L30
                    {
                        //LOG(INFO) << "add";
                        if(RISResult.size() < outLen) RISResult.push_back(*p); else goto make_return;
                        camps.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
                    }
                }
                //LOG(INFO) << "second + count " << (int)RISResult.size() ;
                p++;
            }
            passage++;
        }
        //теперь, если без повторений добрать не получилось, дублируем тизеры.
        //LOG(INFO) << "result count " << (int)RISResult.size() ;
        p = result.begin();
        while(p!=result.end() && ((int)RISResult.size() < informer->capacity))
        {
            //доберём всё за один проход, т.к. result.size > informer.capacity
            //пробуем сначала добрать без повторений.
            if ((*p)->rating > 0.0 || (*p)->branch != EBranchL::L30 )
            {
                if(RISResult.size() < outLen) RISResult.push_back(*p); else goto make_return;
            }
            p++;
        }
        passage = 0;
        while ((passage <  informer->capacity) && ((int)RISResult.size() < informer->capacity))
        {
            hm->clean = true;
            p = result.begin();
            camps.clear();
            //LOG(INFO) << "second -";
            while(p!=result.end() && ((int)RISResult.size() < informer->capacity))
            {
                //доберём всё за один проход, т.к. result.size > informer.capacity
                //пробуем сначала добрать без повторений.
                if(std::find(RISResult.begin(), RISResult.end(), *p) != RISResult.end())
                {
                    if(!camps.count((*p)->campaign_id) && ((*p)->rating <= 0.0))
                    {
                        //LOG(INFO) << "add";
                        if(RISResult.size() < outLen) RISResult.push_back(*p); else goto make_return;
                        camps.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
                    }
                }
                //LOG(INFO) << "second - count " << (int)RISResult.size() ;
                p++;
            }
        }
        p = result.begin();
        while(p!=result.end() && ((int)RISResult.size() < informer->capacity))
        {
            //доберём всё за один проход, т.к. result.size > informer.capacity
            //пробуем сначала добрать без повторений.
            if ((*p)->rating <= 0.0)
            {
                if(RISResult.size() < outLen) RISResult.push_back(*p); else goto make_return;
            }
            p++;
        }

make_return:
        for(p = result.begin(); RISResult.size() <outLen && p != result.end(); ++p)
        {
            RISResult.push_back(*p);
        }

        if(RISResult.size() > outLen)
        {
            result.erase(RISResult.begin() + outLen + 1, RISResult.end());
        }


        for(p = RISResult.begin(); p != RISResult.end(); ++p)
        {
            (*p)->redirect_url =
                                    Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                                     boost::format("id=%s\ninf=%d\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                                     % (*p)->id_int
                                     % informer->id
                                     % rand()
                                     % (*p)->url
                                     % Config::Instance()->server_ip_
                                     % params->location_
                                 ));
        }

    }
}
