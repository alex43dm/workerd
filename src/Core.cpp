#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "../config.h"

#include <ctime>
#include <cstdlib>

#include "Config.h"
#include "Log.h"
#include "Core.h"
#include "DB.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "base64.h"
#include "EBranch.h"

#define CMD_SIZE 8192
#ifndef DUMMY
int HistoryManager::request_processed_ = 0;
int HistoryManager::offer_processed_ = 0;
int HistoryManager::social_processed_ = 0;
#endif // DUMMY
Core::Core()
//    redirect_script_("/redirect")
{
    tid = pthread_self();

    sigset_t es;
    sigfillset(&es);
    pthread_sigmask(SIG_BLOCK, &es, NULL);

    cmd = new char[CMD_SIZE];

    pDb = Config::Instance()->pDb;

    tmpTableName = "tmp" + std::to_string((long long int)getpid()) + std::to_string((long long int)tid);

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
#ifndef DUMMY
    hm = new HistoryManager(tmpTableName);
    hm->initDB();
    hm->vretg = &resultRetargeting;
    //hm->RetargetingOfferStr = pDb->getSqlFile("requests/03.sql");
#endif // DUMMY
    Log::info("[%ld]core start",tid);
}

Core::~Core()
{
    delete []cmd;
#ifndef DUMMY
    delete hm;
    #endif // DUMMY
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
                        % p->gen()
                        % p->url
                        % Config::Instance()->server_ip_
                        % location_
                    ));
    }
};



/** Обработка запроса на показ рекламы с параметрами ``params``.
	Изменён RealInvest Soft */
std::string Core::Process(Params *prms)
{
    Offer::Vector vRIS;

    Log::gdb("Core::Process start");
    Log::gdb("search: %s",prms->getSearch().c_str());
    boost::posix_time::ptime startTime, endTime;//добавлено для отладки, УДАЛИТЬ!!!
    startTime = boost::posix_time::microsec_clock::local_time();

    params = prms;

    if(getInformer() == 0)
    {
        Log::err("there is no informer id: %s", prms->getInformerId().c_str());
        return Config::Instance()->template_error_;
    }

    Log::gdb("[%ld]getInformer: done",tid);

    #ifndef DUMMY
    //load all history async
    hm->getUserHistory(params);

    getOffers(items);
    Log::gdb("[%ld]getOffers: %d done",tid, items.size());

    //wait all history load
    hm->sphinxProcess(items);
    Log::gdb("[%ld]sphinxProcess: done",tid);

    //ris algorithm
    hm->getRetargetingAsyncWait();
    RISAlgorithmRetagreting(resultRetargeting, vOutPut,
                            vRIS.size() == (u_int)informer->capacity ? informer->RetargetingCount : informer->capacity);
    Log::gdb("[%ld]RISAlgorithmRetagreting: vOutPut %ld done",tid, vOutPut.size());

    RISAlgorithm(items, vRIS, informer->capacity);
    Log::gdb("[%ld]RISAlgorithm: vRIS %ld done",tid, vRIS.size());

    if(vRIS.size()==0 || (vRIS.size() < (u_int)informer->capacity && (*vRIS.begin())->type != Offer::Type::banner))
    {
        hm->clean = true;
        Log::gdb("clean offer hostory: by vRIS");

        if(vRIS.size() == 0 && vOutPut.size() == 0)
        {
            getOffers(items,true);
            Log::gdb("[%ld]getAllOffers: %d done",tid, items.size());
        }
    }

    informer->RetargetingCount = vOutPut.size();

    //merge
    if( (vOutPut.size() && (*vOutPut.begin())->type == Offer::Type::banner) ||
       (vRIS.size() && (*vRIS.begin())->type == Offer::Type::banner) )
    {
        if( vOutPut.size() && (*vOutPut.begin())->type == Offer::Type::banner )
        {

        }
        else if( vRIS.size() && (*vRIS.begin())->type == Offer::Type::banner )
        {
            vOutPut.insert(vOutPut.begin(),
                       vRIS.begin(),
                       vRIS.begin()+1);
        }
    }
    else
    {
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

    }
    #else
    getOffers(items);
    Log::gdb("[%ld]getOffers: %d done",tid, items.size());

    for(auto i = items.begin(); i != items.end(); ++i)
    {
        Offer *p = (*i).second;
        p->redirect_url =
            Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%d\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                        % p->id_int
                        % informer->id
                        % p->gen()
                        % p->url
                        % Config::Instance()->server_ip_
                        % params->location_
                    ));
        vOutPut.push_back(p);
    }

    #endif//DUMMY

    std::string ret;
    if (params->json_)
        ret = OffersToJson(vOutPut);
    else
        ret = OffersToHtml(vOutPut, params->getUrl());
//printf("%s\n",ret.c_str());

    Log::info("[%ld]core time: %s %d",tid, boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time() - startTime).c_str(), vOutPut.size());

    return ret;
}

// Сохраняем выданные ссылки в базе данных
void Core::ProcessSaveResults()
{
    request_processed_++;

    if (!params->test_mode_ && informer)
    {

        mongo::BSONObj keywords = mongo::BSONObjBuilder().
#ifndef DUMMY
                                  appendElements(hm->BSON_Keywords()).
#endif // DUMMY
                                  append("search", params->getSearch()).
                                  append("context", params->getContext()).
                                  obj();
#ifndef DUMMY
        hm->updateUserHistory(vOutPut, informer->RetargetingCount);
#endif // DUMMY
        try
        {
            mongo::DB db("log");

            for(auto i = vOutPut.begin(); i != vOutPut.end(); ++i)
            {
                std::tm dt_tm;
                dt_tm = boost::posix_time::to_tm(params->time_);
                mongo::Date_t dt( (mktime(&dt_tm)) * 1000LLU);

                Campaign *c = new Campaign((*i)->campaign_id);

                mongo::BSONObj record = mongo::BSONObjBuilder().genOID().
                                        append("dt", dt).
                                        append("id", (*i)->id).
                                        append("id_int", (*i)->id_int).
                                        append("title", (*i)->title).
                                        append("inf", params->informer_id_).
                                        append("inf_int", informer->id).
                                        append("ip", params->ip_).
                                        append("cookie", params->cookie_id_).
                                        append("social", (*i)->social).
                                        append("token", (*i)->token).
                                        append("type", (*i)->type).
                                        append("isOnClick", (*i)->isOnClick).
                                        append("campaignId", c->guid).
                                        append("campaignId_int", (*i)->campaign_id).
                                        append("campaignTitle", c->title).
                                        append("project", c->project).
                                        append("country", (params->getCountry().empty()?"NOT FOUND":params->getCountry().c_str())).
                                        append("region", (params->getRegion().empty()?"NOT FOUND":params->getRegion().c_str())).
                                        append("keywords", keywords).
                                        append("branch", (*i)->getBranch()).
                                        append("conformity", "place").//(*i)->conformity).
                                        append("matching", (*i)->matching).
                                        obj();
                delete c;

                db.insert(cfg->mongo_log_collection_, record, true);

                offer_processed_ ++;
                if ((*i)->social) social_processed_ ++;
            }
        }
        catch (mongo::DBException &ex)
        {
            Log::err("DBException: insert into log db: %s", ex.what());
        }
    }

    OutPutCampaignMap.clear();

#ifndef DUMMY
    try
    {
        Kompex::SQLiteStatement *p;

        p = new Kompex::SQLiteStatement(pDb->pDatabase);
        std::string sql("DELETE FROM " + tmpTableName + ";");
        p->SqlStatement(sql);
        delete p;
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: delete from %s table: %s", tmpTableName.c_str(), ex.GetString().c_str());
    }
#endif // DUMMY
/*
    for (Offer::it o = items.begin(); o != items.end(); ++o)
    {
        if(o->second)
            delete o->second;
        items.erase(o);
    }
*/
    items.clear();
    result.clear();
    resultRetargeting.clear();

    vOutPut.clear();

    //items.clear();
    if(informer)
        delete informer;
}

Informer *Core::getInformer()
{
    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);

    sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->informerSqlStr.c_str(), params->informer_id_.c_str());
    try
    {
        pStmt->Sql(cmd);
    }
    catch(Kompex::SQLiteException &ex)
    {
        #ifdef DEBUG
        printf("%s\n",cmd);
        #endif // DEBUG
        Log::err("DB error: getInformer: %s: %s", ex.GetString().c_str());
        delete pStmt;
        return 0;
    }

    try
    {
/*
        if(pStmt->GetNumberOfRows())
        {
            Log::err("no informer id: %s",params->informer_id_.c_str());
            return 0;
        }
*/
        while(pStmt->FetchRow())
        {
            informer =  new Informer(pStmt->GetColumnInt64(0),
                                     pStmt->GetColumnInt(1),
                                     pStmt->GetColumnString(2),
                                     pStmt->GetColumnString(3),
                                     pStmt->GetColumnInt64(4),
                                     pStmt->GetColumnInt64(5)
                //                     pStmt->GetColumnInt(6)
                                    );

//            if(!informer->rtgPercentage){
                informer->RetargetingCount  =
                informer->capacity * Config::Instance()->retargeting_by_persents_ / 100;
  //          }else{
    //            informer->RetargetingCount  =
      //          informer->capacity * informer->rtgPercentage / 100;
  //            }
            break;
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        delete pStmt;
        return 0;
    }

    delete pStmt;

    return informer;
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
std::string Core::getGeo()
{
    std::string geo;

    if(params->getCountry().size() || params->getRegion().size())
    {
        Log::gdb("country: %s region: %s",params->getCountry().c_str(),params->getRegion().c_str());

        if(params->getRegion().size())
        {
            try
            {
                Kompex::SQLiteStatement *pStmt;
                pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);
                sqlite3_snprintf(CMD_SIZE, cmd,"SELECT geo.id_cam FROM geoTargeting AS geo \
                INNER JOIN GeoLiteCity AS reg ON geo.id_geo = reg.locId AND reg.city='%q';",
                                 params->getRegion().c_str());

                pStmt->Sql(cmd);

                if(pStmt->GetDataCount() > 0)
                {
                    geo =
                    "INNER JOIN geoTargeting AS geo ON geo.id_cam=cn.id \
                    INNER JOIN GeoLiteCity AS reg ON geo.id_geo = reg.locId AND reg.city='"+params->getRegion()+"'";
                }
                else if(params->getCountry().size())
                {

                    geo =
                    "INNER JOIN geoTargeting AS geo ON geo.id_cam=cn.id \
                    INNER JOIN GeoLiteCity AS reg ON geo.id_geo = reg.locId \
                    AND((reg.country='"+params->getCountry()+"' OR reg.country='O1') AND (reg.city='' OR reg.city='NOT FOUND'))";
                }
                pStmt->FreeQuery();
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Core::getGeo %s error: %s", cmd, ex.GetString().c_str());
            }

        }
        else if(params->getCountry().size())
        {

            geo =
            "INNER JOIN geoTargeting AS geo ON geo.id_cam=cn.id \
            INNER JOIN GeoLiteCity AS reg ON geo.id_geo = reg.locId \
            AND((reg.country='"+params->getCountry()+"' OR reg.country='O1') AND (reg.city='' OR reg.city='NOT FOUND'))";
        }
    }
    return geo;
}
/*
bool Core::getAllOffers(Offer::Map &ret)
{
    sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->offerSqlStr.c_str(),
                     getGeo().c_str(),
                     informer->domainId,
                     informer->domainId,
                     informer->accountId,
                     informer->id,
                     getpid(),
                     tid,
                     informer->id);
    hm->getDeprecatedOffersAsyncWait();
    return getOffers(ret);
}
*/
bool Core::getOffers(Offer::Map &result, bool getAll)
{
    Kompex::SQLiteStatement *pStmt;
    pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);

    #ifndef DUMMY
    if(!getAll)
    {
        sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->offerSqlStr.c_str(),
                         getGeo().c_str(),
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->accountId,
                         informer->accountId,
                         informer->id,
                         informer->id,
                         getpid(),
                         tid,
                         informer->id);
                        hm->getDeprecatedOffersAsyncWait();
    }
    else
    {
        sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->offerSqlStrAll.c_str(),
                         getGeo().c_str(),
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->accountId,
                         informer->accountId,
                         informer->id,
                         informer->id,
                         informer->id,
                         informer->capacity);
    }
    #else
    sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->offerSqlStr.c_str(),
                     getGeo().c_str(),
                     informer->domainId,
                     informer->domainId,
                     informer->accountId,
                     informer->id,
                     informer->capacity);
    #endif

#ifdef DEBUG
    printf("%s\n",cmd);
#endif // DEBUG

    try
    {
        pStmt->Sql(cmd);
    }
    catch(Kompex::SQLiteException &ex)
    {
        printf("%s\n", cmd);
        Log::err("DB error: getOffers: %s", ex.GetString().c_str());
        delete pStmt;
        return false;
    }

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
            off->branch = EBranchL::L30;

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

    if(items.size() == 0)
    {
        return std::string();
    }

    //для отображения передаётся или один баннер, или вектор тизеров. это и проверяем
    if( (*items.begin())->type == Offer::Type::banner )
    {
        // Получаем HTML-код информера для отображение баннера
        informer_html =
            boost::str(boost::format(Config::Instance()->template_banner_)
                       % informer->bannersCss
                       % Config::Instance()->swfobject_
                       % OffersToJson(items));
    }
    else
    {
        // Получаем HTML-код информера для отображение тизера
        informer_html =
            boost::str(boost::format(Config::Instance()->template_teaser_)
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
    if (offer->type == Offer::Type::banner)
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
#define FULL RISResult.size() >= outLen
void Core::RISAlgorithm(const Offer::Map &items, Offer::Vector &RISResult, unsigned outLen)
{
    Offer::itV p;
    Offer::Vector result;
    Offer::MapRate resultAll;

    RISResult.clear();

    if(items.size() == 0)
    {
        return;
    }

    //sort by rating
    for(auto i = items.begin(); i != items.end(); i++)
    {
        resultAll.insert(Offer::PairRate((*i).second->rating, (*i).second));
    }

    //vector by rating
    for(auto i = resultAll.begin(); i != resultAll.end(); ++i)
    {
        if((*i).second->type == Offer::Type::teazer)
        {
            teasersCount++;
            teasersMediumRating += (*i).second->rating;
        }
        else if((*i).second->type == Offer::Type::banner)
        {
            RISResult.push_back((*i).second);
            Log::gdb("banner get");
            goto links_make;
        }
        //add if all not social and not social offer(skip social)
        if(!all_social && !(*i).second->social)
        {
            result.push_back((*i).second);
        }
    }
    //medium reting
    teasersMediumRating /= teasersCount;

    //size check
    if(result.size() < outLen)
    {
        #ifndef DUMMY
        hm->clean = true;
        #endif // DUMMY
        Log::warn("RISAlgorithm: result size less then %d: clean history", outLen);
    }

    //check is all social
    if(all_social)
    {
        #ifndef DUMMY
        hm->clean = true;
        #endif // DUMMY
        Log::warn("RISAlgorithm: all social: clean history");
    }

    //add teaser when teaser unique id and with company unique and rating > 0
    for(p = result.begin(); p != result.end(); ++p)
    {
        if(!OutPutCampaignMap.count((*p)->campaign_id)
                && (*p)->rating > 0.0
                && std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
        {
            if(FULL)
            {
                goto links_make;
            }

            RISResult.push_back(*p);
            OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
        }
    }
/*
    //user history view clean
    #ifndef DUMMY
    hm->clean = true;
    Log::warn("RISAlgorithm: clean offer history");
    #endif // DUMMY
*/
    if(FULL)
    {
        goto links_make;
    }

    //add teaser when teaser unique id and with company unique and with any rating
    for(p = result.begin(); p!=result.end() && RISResult.size() < outLen; ++p)
    {
        if(!OutPutCampaignMap.count((*p)->campaign_id)
                && std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
        {
            if(FULL)
            {
                goto links_make;
            }

            RISResult.push_back(*p);
            OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
        }
    }

    if(FULL)
    {
        goto links_make;
    }

    //add teaser when teaser unique id and with id unique and with any rating
    for(p = result.begin(); p != result.end() && RISResult.size() < outLen; ++p)
    {
        if(std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
        {
            if(FULL)
            {
                goto links_make;
            }

            RISResult.push_back(*p);
            OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
        }
    }

    if(FULL)
    {
        goto links_make;
    }

    //expand to return size
      while(RISResult.size() < outLen)
     {
         for(p = result.begin(); RISResult.size() < outLen && p != result.end(); ++p)
         {
             RISResult.push_back(*p);
             OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
         }
     }

     //user history view clean
     #ifndef DUMMY
     hm->clean = true;
     Log::warn("RISAlgorithm: clean offer history");
     #endif // DUMMY

links_make:
    //redirect links make
    for(p = RISResult.begin(); p != RISResult.end(); ++p)
    {
        (*p)->redirect_url =
            Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%d\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                        % (*p)->id_int
                        % informer->id
                        % (*p)->gen()
                        % (*p)->url
                        % Config::Instance()->server_ip_
                        % params->location_
                    ));
    }
}
//
void Core::RISAlgorithmRetagreting(const Offer::Vector &result, Offer::Vector &RISResult, unsigned outLen)
{
    RISResult.clear();

    if(result.size() == 0)
    {
        return;
    }

    //add teaser when teaser unique id and with company unique and rating > 0
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if((*p)->type == Offer::Type::banner)
        {
            RISResult.clear();
            RISResult.push_back(*p);
            OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
            goto make_return;
        }

        if(!OutPutCampaignMap.count((*p)->campaign_id)
            && (*p)->rating > 0.0
            && std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
        {
            if(RISResult.size() < outLen)
            {
                RISResult.push_back(*p);
                OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
            }
            else
            {
                goto make_return;
            }
        }
    }

    //add teaser when teaser unique id and with company unique and any rating
    if(RISResult.size() < outLen)
    {
        for(auto p = result.begin(); p!=result.end() && RISResult.size() < outLen; ++p)
        {
            if(!OutPutCampaignMap.count((*p)->campaign_id)
               && std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
            {
                if(RISResult.size() < outLen)
                {
                    RISResult.push_back(*p);
                    OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
                }
                else
                {
                    goto make_return;
                }
            }
        }
    }

    //add teaser when teaser unique id
    if(!Config::Instance()->retargeting_unique_by_campaign_)
    {
        if(RISResult.size() < outLen)
        {
            for(auto p = result.begin(); p != result.end() && RISResult.size() < outLen; ++p)
            {
                if(std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
                {
                    if(RISResult.size() < outLen)
                    {
                        RISResult.push_back(*p);
                        OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
                    }
                    else
                    {
                        goto make_return;
                    }
                }
            }
        }
    }

make_return:
    //redirect links make
    for(auto p = RISResult.begin(); p != RISResult.end(); ++p)
    {
        (*p)->redirect_url =
            Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%d\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                        % (*p)->id_int
                        % informer->id
                        % (*p)->gen()
                        % (*p)->url
                        % Config::Instance()->server_ip_
                        % params->location_
                    ));
    }
}
