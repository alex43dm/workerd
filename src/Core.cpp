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
{
    tid = pthread_self();

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
        /*
        sqlite3_snprintf(CMD_SIZE, cmd, "CREATE INDEX IF NOT EXISTS idx_%s_id ON %s(id);",
                         tmpTableName.c_str(), tmpTableName.c_str());
        p->SqlStatement(cmd);
        */
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" error: create tmp table: %s"<< ex.GetString()<<std::endl;
        exit(1);
    }
    delete p;
#ifndef DUMMY
    hm = new HistoryManager();
    hm->initDB();
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
    std::string informerId;
    std::string location_;
public:
    GenerateRedirectLink( std::string &informerId,
                          const std::string &location)
        : informerId(informerId),
          location_(location) { }

    void operator ()(Offer *p)
    {
        p->redirect_url =
            Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%d\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                        % p->id
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

//    Log::gdb("Core::Process start");
    startCoreTime = boost::posix_time::microsec_clock::local_time();

    params = prms;

    if(!getInformer())
    {
        std::clog<<"there is no informer id: "<<prms->getInformerId()<<std::endl;
        return Config::Instance()->template_error_;
    }

//    Log::gdb("[%ld]getInformer: done",tid);

#ifndef DUMMY
    //init history search
    hm->startGetUserHistory(params, informer);

    //load all history async
    //hm->getDeprecatedOffersAsync();

    //get campaign list
    getCampaign();

    hm->getRetargetingAsync();

    hm->getTailOffersAsync();

    getOffers();

    //process history
    hm->sphinxProcess(items);

    //set tail
    hm->getTailOffersAsyncWait();
    hm->moveUpTailOffers(items, teasersMaxRating);

    hm->getRetargetingAsyncWait();
    RISAlgorithmRetagreting(hm->vretg, vOutPut);

    RISAlgorithm(items, vRIS);

    RetargetingCount = vOutPut.size();

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

    for(auto i = items.begin(); i != items.end(); ++i)
    {
        Offer *p = (*i).second;
        p->redirect_url =
            Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%s\ntoken=%X\nurl=%s\nserver=%s\nloc=%s")
                        % p->id
                        % params->informer_id_
                        % p->gen()
                        % p->url
                        % Config::Instance()->server_ip_
                        % params->location_
                    ));
        vOutPut.push_back(p);
    }

#endif//DUMMY

    std::string ret;

    if(!vOutPut.empty())
    {
        if (params->json_)
            ret = OffersToJson(vOutPut);
        else
            ret = OffersToHtml(vOutPut, params->getUrl());
    }
    else
    {
        ret = cfg->template_error_;
    }
//printf("%s\n",ret.c_str());


    endCoreTime = boost::posix_time::microsec_clock::local_time();

    return ret;
}

void Core::log()
{
    std::clog<<"["<<tid<<"]";

   if(cfg->logCoreTime)
        std::clog<<" core time:"<< boost::posix_time::to_simple_string(endCoreTime - startCoreTime);

    if(cfg->logOutPutSize)
        std::clog<<" out:"<<vOutPut.size();

    if(cfg->logIP)
        std::clog<<" ip:"<<params->getIP();

    if(cfg->logCountry)
        std::clog<<" country:"<<params->getCountry();

    if(cfg->logRegion)
        std::clog<<" region:"<<params->getRegion();

    if(cfg->logCookie)
        std::clog<<" cookie:"<<params->getCookieId();

    if(cfg->logContext)
        std::clog<<" context:"<<params->getContext();

    if(cfg->logSearch)
        std::clog<<" search:"<<params->getSearch();

    if(cfg->logInformerId)
        std::clog<<" informer id:"<<informer->id;

    if(cfg->logLocation)
        std::clog<<" location:"<<params->getLocation();

    if(cfg->logOutPutOfferIds || cfg->logRetargetingOfferIds)
        std::clog<<" key:"<<params->getUserKey();

}

// Сохраняем выданные ссылки в базе данных
void Core::ProcessSaveResults()
{
    request_processed_++;

    log();

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
        //cycle view
        if(items.size() <= (u_int)informer->capacity * 2)
        {
//            Log::gdb("set tail");
            hm->clean = true;
            hm->setTailOffers(items,vOutPut);
        }

        hm->updateUserHistory(vOutPut, RetargetingCount);
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
                                        append("type", Offer::typeToString((*i)->type)).
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

                db.insert(cfg->mongo_log_collection_, record, true);

                delete c;

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

    for (Offer::it o = items.begin(); o != items.end(); ++o)
    {
        if(o->second)
            delete o->second;
//        items.erase(o);
    }

    items.clear();
    result.clear();

    vOutPut.clear();

    if(informer)
        delete informer;

    std::clog<<std::endl;
}

bool Core::getInformer()
{
    bool ret = false;
    Kompex::SQLiteStatement *pStmt;

    informer = nullptr;

    pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);

    sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->informerSqlStr.c_str(), params->informer_id_.c_str());

    try
    {
        /*
                if(pStmt->GetNumberOfRows())
                {
                    Log::err("no informer id: %s",params->informer_id_.c_str());
                    return 0;
                }
        */
        pStmt->Sql(cmd);

        while(pStmt->FetchRow())
        {
            informer =  new Informer(pStmt->GetColumnInt64(0),
                                     pStmt->GetColumnInt(1),
                                     pStmt->GetColumnString(2),
                                     pStmt->GetColumnString(3),
                                     pStmt->GetColumnInt64(4),
                                     pStmt->GetColumnInt64(5),
                                     pStmt->GetColumnDouble(6),
                                     pStmt->GetColumnDouble(7),
                                     pStmt->GetColumnDouble(8),
                                     pStmt->GetColumnDouble(9),
                                     pStmt->GetColumnInt(10),
                                     pStmt->GetColumnBool(11)
                                    );
            ret = true;
            break;
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" error: " <<ex.GetString()<<std::endl;
    }

    delete pStmt;

    return ret;
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
                INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_locId_city ON geo.id_geo = reg.locId AND reg.city='%q';",
                                 params->getRegion().c_str());

                pStmt->Sql(cmd);

                if(pStmt->GetDataCount() > 0)
                {
                    geo =
                        "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id \
                    INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId AND reg.city='"+params->getRegion()+"'";
                }
                else if(params->getCountry().size())
                {

                    geo =
                        "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id \
                    INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId \
                    AND((reg.country='"+params->getCountry()+"' OR reg.country='O1') AND (reg.city='' OR reg.city='NOT FOUND'))";
                }
                pStmt->FreeQuery();
                delete pStmt;
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Core::getGeo %s error: %s", cmd, ex.GetString().c_str());
            }

        }
        else if(params->getCountry().size())
        {

            geo =
                "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id \
            INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId \
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
bool Core::getOffers(bool getAll)
{
    Kompex::SQLiteStatement *pStmt;
    bool ret = true;
    all_social = false;

#ifndef DUMMY
    if(!getAll)
    {
        sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->offerSqlStr.c_str(),
                         tmpTableName.c_str(),
                         params->getUserKeyLong(),
                         informer->id);
        //hm->getDeprecatedOffersAsyncWait();
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

    pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);
    try
    {
        teasersCount = 0;
        teasersMediumRating = 0;
        teasersMaxRating = 0;

        pStmt->Sql(cmd);
        while(pStmt->FetchRow())
        {

            if(items.count(pStmt->GetColumnInt64(0)) > 0)
            {
                continue;
            }

            Offer *off = new Offer(pStmt->GetColumnString(1),
                                   pStmt->GetColumnInt64(0),
                                   pStmt->GetColumnString(2),
                                   pStmt->GetColumnString(3),
                                   pStmt->GetColumnString(4),
                                   pStmt->GetColumnString(5),
                                   pStmt->GetColumnString(6),
                                   pStmt->GetColumnString(7),
                                   pStmt->GetColumnInt64(8),
                                   pStmt->GetColumnBool(9),
                                   pStmt->GetColumnInt(10),
                                   pStmt->GetColumnDouble(11),
                                   pStmt->GetColumnBool(12),
                                   pStmt->GetColumnInt(13),
                                   pStmt->GetColumnInt(14),
                                   pStmt->GetColumnInt(15),
                                   pStmt->GetColumnBool(16),
                                   pStmt->GetColumnString(17),
                                   pStmt->GetColumnInt(18)
                                  );

            if(!off->social)
                all_social = false;

            if(off->rating > teasersMaxRating)
            {
                teasersMaxRating = off->rating;
            }
            items.insert(Offer::Pair(off->id_int,off));
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<tid<<"] error: "<<__func__
                 <<ex.GetString()
                 <<std::endl;

        ret = false;
    }


    pStmt->FreeQuery();
    delete pStmt;

    return ret;
}

bool Core::getCampaign()
{
    bool ret = false;
    Kompex::SQLiteStatement *pStmt;

    sqlite3_snprintf(CMD_SIZE, cmd, Config::Instance()->campaingSqlStr.c_str(),
                         tmpTableName.c_str(),
                         getGeo().c_str(),
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->accountId,
                         informer->accountId,
                         informer->accountId,
                         informer->id,
                         informer->id,
                         informer->id,
                         informer->blocked ? " AND ca.social=1 " : ""
                         );

    pStmt = new Kompex::SQLiteStatement(pDb->pDatabase);

#ifdef DEBUG
    printf("%s\n",cmd);
#endif // DEBUG

    try
    {
        pStmt->Sql(cmd);
        ret = true;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<tid<<"] error: "<<__func__
                 <<ex.GetString()
                 <<std::endl;
        ret = false;
    }


    pStmt->FreeQuery();
    delete pStmt;

    return ret;
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
#define FULL RISResult.size() >= informer->capacity
void Core::RISAlgorithm(const Offer::Map &items, Offer::Vector &RISResult)
{
    Offer::itV p;
    Offer::Vector result;
    Offer::MapRate resultAll;
    unsigned loopCount;

//    RISResult.clear();

    if( items.size() == 0 )
    {
        std::clog<<"["<<tid<<"]"<<typeid(this).name()<<"::"<<__func__<< "error items size: 0"<<std::endl;
        return;
    }

    //sort by rating
    for(auto i = items.begin(); i != items.end(); i++)
    {
        if((*i).second)
        {
            resultAll.insert(Offer::PairRate((*i).second->rating, (*i).second));
        }
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
    if(result.size() <= informer->capacity)
    {
#ifndef DUMMY
        hm->clean = true;
#endif // DUMMY
        std::clog<<"["<<tid<<"]"<<typeid(this).name()<<"::"<<__func__<< "result size less or equal: "<<result.size()<<" outLen: "<<informer->capacity<<", clean history"<<std::endl;
    }

    //check is all social
    if(all_social)
    {
#ifndef DUMMY
        hm->clean = true;
#endif // DUMMY
        std::clog<<"["<<tid<<"]"<<typeid(this).name()<<"::"<<__func__<< "all social: clean history"<<std::endl;
    }

    //add teaser when teaser unique id and with company unique and rating > 0
    for(p = result.begin(); p != result.end(); ++p)
    {
        if((!OutPutCampaignMap.count((*p)->campaign_id) < (*p)->unique_by_campaign)
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
    for(p = result.begin(); p!=result.end() && RISResult.size() < informer->capacity; ++p)
    {
        if((!OutPutCampaignMap.count((*p)->campaign_id) < (*p)->unique_by_campaign)
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
    for(p = result.begin(); p != result.end() && RISResult.size() < informer->capacity; ++p)
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
    loopCount = RISResult.size();
    for(p = result.begin(); loopCount < informer->capacity && p != result.end(); ++p, loopCount++)
    {
        RISResult.push_back(*p);
        OutPutCampaignMap.insert(std::pair<const long, long>((*p)->campaign_id,(*p)->campaign_id));
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
                        boost::format("id=%s\ninf=%s\ntoken=%s\nurl=%s\nserver=%s\nloc=%s")
                        % (*p)->id
                        % params->informer_id_
                        % (*p)->gen()
                        % (*p)->url
                        % Config::Instance()->server_ip_
                        % params->location_
                    ));
    }
}
//
void Core::RISAlgorithmRetagreting(const Offer::Vector &result, Offer::Vector &RISResult)
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

        if((!OutPutCampaignMap.count((*p)->campaign_id) < (*p)->unique_by_campaign)
                && (*p)->rating > 0.0
                && std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
        {
            if(RISResult.size() < informer->retargeting_capacity)
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
    if(RISResult.size() < informer->retargeting_capacity)
    {
        for(auto p = result.begin(); p!=result.end() && RISResult.size() < informer->retargeting_capacity; ++p)
        {
        if((!OutPutCampaignMap.count((*p)->campaign_id) < (*p)->unique_by_campaign)
                    && std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
            {
                if(RISResult.size() < informer->retargeting_capacity)
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
        if(RISResult.size() < informer->retargeting_capacity)
        {
            for(auto p = result.begin(); p != result.end() && RISResult.size() < informer->retargeting_capacity; ++p)
            {
                if(std::find(RISResult.begin(), RISResult.end(), *p) == RISResult.end())
                {
                    if(RISResult.size() < informer->retargeting_capacity)
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

make_return:
    //redirect links make
    for(auto p = RISResult.begin(); p != RISResult.end(); ++p)
    {
        (*p)->redirect_url =
            Config::Instance()->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%s\ntoken=%s\nurl=%s\nserver=%s\nloc=%s")
                        % (*p)->id
                        % params->informer_id_
                        % (*p)->gen()
                        % (*p)->url
                        % Config::Instance()->server_ip_
                        % params->location_
                    ));
    }
}

