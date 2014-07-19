#include <signal.h>

#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

//----------------------------Retargeting---------------------------------------
void HistoryManager::getRetargeting()
{
    std::string ids;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];
    Offer::MapRate rateMap;

    if(params->newClient)
    {
        return;
    }

    if(!pRetargeting->exists(key))
    {
        return;
    }

    ids = pRetargeting->getRange(key);

    if(ids.empty())
    {
        return;
    }


    sqlite3_snprintf(sizeof(buf), buf, cfg->retargetingOfferSqlStr.c_str(),
                     params->getUserKeyLong(),
                     ids.c_str());

    try
    {
        pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
        pStmt->Sql(buf);

        while(pStmt->FetchRow())
        {
            Offer *off = new Offer(pStmt->GetColumnString(1),     //guid
                                   pStmt->GetColumnInt64(0),    //id
                                   pStmt->GetColumnString(2),   //title
                                   pStmt->GetColumnString(3),   //description
                                   pStmt->GetColumnString(4),   //url
                                   pStmt->GetColumnString(5),   //image
                                   pStmt->GetColumnString(6),   //swf
                                   pStmt->GetColumnInt64(7),    //campaignId
                                   pStmt->GetColumnInt(8),      //type
                                   pStmt->GetColumnDouble(9),   //rating
                                   pStmt->GetColumnInt(10),    //uniqueHits
                                   pStmt->GetColumnInt(11),     //height
                                   pStmt->GetColumnInt(12),     //width
                                   pStmt->GetColumnBool(13),    //isOnClick
                                   pStmt->GetColumnBool(14),    //social
                                   pStmt->GetColumnString(15),  //campaign_guid
                                   pStmt->GetColumnInt(16),      //offer_by_campaign_unique
                                   true
                                  );

            rateMap.insert(Offer::PairRate(off->rating,off));
        }
        pStmt->FreeQuery();
        delete pStmt;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" Kompex::SQLiteException: "<<ex.GetString()<<std::endl;
    }

    RISAlgorithmRetagreting(rateMap);
}
//-------------------------------------------------------------------------------------------------------------------
void HistoryManager::RISAlgorithmRetagreting(const Offer::MapRate &result)
{
    std::multiset<unsigned long long> OutPutCampaignSet;
    std::set<unsigned long long> OutPutOfferSet;

    if(result.size() == 0)
    {
        return;
    }

    //add teaser when teaser unique id and with company unique
    for(auto p = result.begin(); p != result.end(); ++p)
    {
        if(OutPutCampaignSet.count((*p).second->campaign_id) == 0
                && OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            (*p).second->branch = EBranchL::L32;
            vRISRetargetingResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);
            OutPutCampaignSet.insert((*p).second->campaign_id);

            if(vRISRetargetingResult.size() >= inf->retargeting_capacity)
                return;
        }
    }

    //add teaser when teaser unique id
    for(auto p = result.begin(); p!=result.end(); ++p)
    {
        if(OutPutCampaignSet.count((*p).second->campaign_id) < (*p).second->unique_by_campaign
                && OutPutOfferSet.count((*p).second->id_int) == 0)
        {
            (*p).second->branch = EBranchL::L33;
            vRISRetargetingResult.push_back((*p).second);
            OutPutOfferSet.insert((*p).second->id_int);

            if(vRISRetargetingResult.size() >= inf->retargeting_capacity)
                return;
        }
    }
}

void *HistoryManager::getRetargetingEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;

    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = signalHanlerRetargeting;
    sigaddset(&sact.sa_mask, SIGPIPE);

    if( sigaction(SIGPIPE, &sact, 0) )
    {
        std::clog<<"error set sigaction"<<std::endl;
    }

    pthread_sigmask(SIG_SETMASK, &sact.sa_mask, NULL);

    h->getRetargeting();
    return NULL;
}

bool HistoryManager::getRetargetingAsync()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetRetargetingAsync, pAttr, &this->getRetargetingEnv, this))
    {
        std::clog<<"creating thread failed"<<std::endl;
    }

    pthread_attr_destroy(pAttr);

    return true;
}

void HistoryManager::getRetargetingAsyncWait()
{

    if(params->newClient)
    {
        return;
    }

    pthread_join(thrGetRetargetingAsync, 0);
    return;
}

void HistoryManager::signalHanlerRetargeting(int sigNum)
{
    std::clog<<__func__<<"get signal: "<<sigNum<<std::endl;
}
