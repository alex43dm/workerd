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

    //get geo
    getGeo(params->getCountry(), params->getRegion());

    //get campaign list
    getCampaign();

    endCampaignTime = boost::posix_time::microsec_clock::local_time();

    getOffers(items, params->getUserKeyLong());

    //process history
    hm->sphinxProcess(items,teasersMaxRating);

    //set tail
    hm->moveUpTailOffers(items, teasersMaxRating);

    RISAlgorithm(items);

    mergeWithRetargeting();

#else
    getOffers(items);

    for(auto i = items.begin(); i != items.end(); ++i)
    {
        vResult.push_back(p);
    }

#endif//DUMMY

    resultHtml();

    endCoreTime = boost::posix_time::microsec_clock::local_time();

    //printf("%s\n",retHtml.c_str());
    return retHtml;
}
//-------------------------------------------------------------------------------------------------------------------
void Core::log()
{
    if(cfg->toLog())
    {
        std::clog<<"["<<tid<<"]";
    }

    if(cfg->logCoreTime)
    {
        std::clog<<" core time:"<< boost::posix_time::to_simple_string(endCoreTime - startCoreTime)
        <<" sphinx:"<<hm->isSphinxProcessed();
    }

    if(cfg->logOutPutSize)
        std::clog<<" out:"<<vResult.size();

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

        if(hm->clean)
        {
            std::clog<<"[clean],";
        }

        std::clog<<" output ids:[";
        for(auto it = vResult.begin(); it != vResult.end(); ++it)
        {
            std::clog<<" "<<(*it)->id<<" "<<(*it)->id_int
            <<" hits:"<<(*it)->uniqueHits
            <<" rate:"<<(*it)->rating
            <<" cam:"<<(*it)->campaign_id
            <<" branch:"<<(*it)->getBranch();
        }
        std::clog<<"]";
    }
/*
    if(cfg->logCoreTime)
    {
        std::clog<<" camp time:"<< boost::posix_time::to_simple_string(endCampaignTime - startCoreTime);
    }*/
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
//                                  appendElements(hm->BSON_Keywords()).
#endif // DUMMY
                                  append("search", params->getSearch()).
                                  append("context", params->getContext()).
                                  obj();
        try
        {
            mongo::DB db("log");

            for(auto i = vResult.begin(); i != vResult.end(); ++i)
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

#ifndef DUMMY
        hm->updateUserHistory(items, vResult, all_social);
#endif // DUMMY

    //clear tmp values: informer & temp table
    clearTmp();

    for (Offer::it o = items.begin(); o != items.end(); ++o)
    {
        if(o->second)
            delete o->second;
    }
    //clear all offers map
    items.clear();
    //clear output offers vector
    vResult.clear();
#ifndef DUMMY
    OutPutCampaignSet.clear();
    OutPutOfferSet.clear();
#endif // DUMMY

    if(cfg->toLog())
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
void Core::resultHtml()
{
    if(!vResult.empty())
    {
        if (params->json_)
            retHtml = OffersToJson(vResult);
        else
            retHtml = OffersToHtml(vResult, params->getUrl());
    }
    else
    {
        retHtml = cfg->template_error_;
    }
}
//-------------------------------------------------------------------------------------------------------------------
#ifndef DUMMY
void Core::RISAlgorithm(const Offer::Map &items)
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
            if((*i).second->type == Offer::Type::banner && vResult.size() == 0)
            {
                (*i).second->branch = EBranchL::L1;
                vResult.insert(vResult.begin(),(*i).second);
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

    if(result.size() <= informer->capacity * 2)
    {
        hm->clean = true;
    }

    //teaser by unique id and company
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if(OutPutCampaignSet.count((*p).second->campaign_id) < (*p).second->unique_by_campaign
                && OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            if((*p).second->branch ==  EBranchL::L30) (*p).second->branch = EBranchL::L2;
            vResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);

            if(vResult.size() >= informer->capacity)
                return;
        }
    }

    //teaser by unique id
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if(OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            (*p).second->branch = EBranchL::L3;
            vResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);

            if(vResult.size() >= informer->capacity)
                return;
        }
    }

    //expand to return size
    loopCount = vResult.size();
    for(auto p = result.begin(); loopCount < informer->capacity && p != result.end(); ++p, loopCount++)
    {
        (*p).second->branch = EBranchL::L4;
        vResult.push_back((*p).second);
    }

    //user history view clean
    hm->clean = true;
    //std::clog<<"["<<tid<<"] "<<__func__<<" clean offer history"<<std::endl;
}
//-------------------------------------------------------------------------------------------------------------------
void Core::mergeWithRetargeting()
{
    hm->getRetargetingAsyncWait();

    if(vResult.size()
       && (*vResult.begin())->type != Offer::Type::banner
       && hm->vRISRetargetingResult.size())
    {
        vResult.insert(vResult.begin(),hm->vRISRetargetingResult.begin(),hm->vRISRetargetingResult.end());
        vResult.erase(vResult.end()-hm->vRISRetargetingResult.size(),vResult.end());
    }
}
#endif // DUMMY
