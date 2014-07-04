#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <ctime>
#include <cstdlib>

#include "../config.h"

#include "Config.h"
#include "Core.h"
#include "DB.h"
#include "base64.h"

#ifndef DUMMY
int HistoryManager::request_processed_ = 0;
int HistoryManager::offer_processed_ = 0;
int HistoryManager::social_processed_ = 0;
#endif // DUMMY

Core::Core()
{
    tid = pthread_self();

#ifndef DUMMY
    hm = new HistoryManager();
    hm->initDB();
#endif // DUMMY

    std::clog<<"["<<tid<<"]core start"<<std::endl;
}
//-------------------------------------------------------------------------------------------------------------------
Core::~Core()
{
#ifndef DUMMY
    delete hm;
#endif // DUMMY
}
//-------------------------------------------------------------------------------------------------------------------
std::string Core::Process(Params *prms)
{
    Offer::Vector vRIS;

//    Log::gdb("Core::Process start");
    startCoreTime = boost::posix_time::microsec_clock::local_time();

    params = prms;

    if(!getInformer(params->informer_id_))
    {
        std::clog<<"there is no informer id: "<<prms->getInformerId()<<std::endl;
        return Config::Instance()->template_error_;
    }

#ifndef DUMMY
    //init history search
    hm->startGetUserHistory(params, informer);

    hm->getRetargetingAsync();

    hm->getTailOffersAsync();

    //get geo
    getGeo(params->getCountry(), params->getRegion());

    //get campaign list
    getCampaign();

    getOffers(items,params->getUserKeyLong());

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
//-------------------------------------------------------------------------------------------------------------------
void Core::log()
{
    std::clog<<"["<<tid<<"]";

    if(cfg->logCoreTime)
    {
        std::clog<<" core time:"<< boost::posix_time::to_simple_string(endCoreTime - startCoreTime)
        <<" use sphinx:"<<hm->isSphinxProcessed();
    }

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
    {
        std::clog<<" key:"<<params->getUserKey();
        std::clog<<" offers: total:"<<offersTotal;
    }
}
//-------------------------------------------------------------------------------------------------------------------
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
        hm->updateUserHistory(items, vOutPut, RetargetingCount);
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
                                        append("id_int", (unsigned int)(*i)->id_int).
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
                                        append("campaignId_int", (unsigned int)(*i)->campaign_id).
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

    //clear tmp values: informer & temp table
    clearTmp();

    for (Offer::it o = items.begin(); o != items.end(); ++o)
    {
        if(o->second)
            delete o->second;
//        items.erase(o);
    }
    //clear all offers map
    items.clear();
    //clear output offers vector
    vOutPut.clear();

    OutPutCampaignSet.clear();
    OutPutOfferSet.clear();

    std::clog<<std::endl;
}
//-------------------------------------------------------------------------------------------------------------------
std::string Core::OffersToHtml(Offer::Vector &items, const std::string &url)
{
    std::string informer_html;

    //для отображения передаётся или один баннер, или вектор тизеров. это и проверяем
    if( (*items.begin())->type == Offer::Type::banner )
    {
        // Получаем HTML-код информера для отображение баннера
        informer_html =
            boost::str(boost::format(cfg->template_banner_)
                       % informer->bannersCss
                       % cfg->swfobject_
                       % OffersToJson(items));
    }
    else
    {
        // Получаем HTML-код информера для отображение тизера
        informer_html =
            boost::str(boost::format(cfg->template_teaser_)
                       % informer->teasersCss
                       % OffersToJson(items)
                       % informer->capacity
                       % url);
    }

    return informer_html;
}
//-------------------------------------------------------------------------------------------------------------------
std::string Core::OffersToJson(Offer::Vector &items)
{
    std::stringstream json;
    json << "[";
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        if (it != items.begin())
            json << ",";

        (*it)->redirect_url =
            cfg->redirect_script_ + "?" + base64_encode(boost::str(
                        boost::format("id=%s\ninf=%s\ntoken=%s\nurl=%s\nserver=%s\nloc=%s")
                        % (*it)->id
                        % params->informer_id_
                        % (*it)->gen()
                        % (*it)->url
                        % cfg->server_ip_
                        % params->location_
                    ));

        json << (*it)->toJson();
    }

    json << "]";

    return json.str();
}
//-------------------------------------------------------------------------------------------------------------------
void Core::RISAlgorithm(const Offer::Map &items, Offer::Vector &RISResult)
{
    Offer::MapRate result;
    unsigned loopCount;

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
            if((*i).second->type == Offer::Type::banner)
            {
                RISResult.clear();
                RISResult.insert(RISResult.begin(),(*i).second);
                return;
            }

            if(!all_social)
            {
                if(!(*i).second->social)
                {
                    result.insert(Offer::PairRate((*i).second->rating, (*i).second));
                }
            }
            else
            {
                result.insert(Offer::PairRate((*i).second->rating, (*i).second));
            }
        }
    }
#ifndef DUMMY
    if(result.size() <= informer->capacity * 2)
    {
        hm->clean = true;
    }
#endif // DUMMY

    //add teaser when teaser unique id and with company unique and rating > 0
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if(OutPutCampaignSet.count((*p).second->campaign_id) < (*p).second->unique_by_campaign
                && OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            RISResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);

            if(RISResult.size() >= informer->capacity)
                return;
        }
    }

    //add teaser when teaser unique id and with company unique and with any rating
    for(auto p = result.begin(); p!=result.end(); ++p)
    {
        if(OutPutCampaignSet.count((*p).second->campaign_id) < (*p).second->unique_by_campaign
                && OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            RISResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);

            if(RISResult.size() >= informer->capacity)
                return;
        }
    }

    //add teaser when teaser unique id and with id unique and with any rating
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if(OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            RISResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);

            if(RISResult.size() >= informer->capacity)
                return;
        }
    }

    //expand to return size
    loopCount = RISResult.size();
    for(auto p = result.begin(); loopCount < informer->capacity && p != result.end(); ++p, loopCount++)
    {
        RISResult.push_back((*p).second);
    }

    //user history view clean
#ifndef DUMMY
    hm->clean = true;
    std::clog<<"["<<tid<<"] "<<__func__<<" clean offer history"<<std::endl;
#endif // DUMMY
}
//-------------------------------------------------------------------------------------------------------------------
void Core::RISAlgorithmRetagreting(const Offer::Vector &result, Offer::Vector &RISResult)
{
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
            RISResult.insert(RISResult.begin(),*p);
            return;
        }

        if(OutPutCampaignSet.count((*p)->campaign_id) < (*p)->unique_by_campaign
                && OutPutOfferSet.count((*p)->id_int) == 0)
        {
            RISResult.push_back(*p);
            OutPutOfferSet.insert((*p)->id_int);
            OutPutCampaignSet.insert((*p)->campaign_id);

            if(RISResult.size() >= informer->retargeting_capacity)
                return;
        }
    }

    //add teaser when teaser unique id and with company unique and any rating
    if(RISResult.size() < informer->retargeting_capacity)
    {
        for(auto p = result.begin(); p!=result.end(); ++p)
        {
            if(OutPutCampaignSet.count((*p)->campaign_id) < (*p)->unique_by_campaign
                    && OutPutOfferSet.count((*p)->id_int) == 0)
            {
                RISResult.push_back(*p);
                OutPutOfferSet.insert((*p)->id_int);
                OutPutCampaignSet.insert((*p)->campaign_id);

                if(RISResult.size() >= informer->retargeting_capacity)
                    return;
            }
        }
    }

    //add teaser when teaser unique id
    if(RISResult.size() < informer->retargeting_capacity)
    {
        for(auto p = result.begin(); p != result.end(); ++p)
        {
            if(OutPutOfferSet.count((*p)->id_int) == 0)
            {
                RISResult.push_back(*p);
                OutPutOfferSet.insert((*p)->id_int);
                OutPutCampaignSet.insert((*p)->campaign_id);

                if(RISResult.size() >= informer->retargeting_capacity)
                    return;
            }
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------
