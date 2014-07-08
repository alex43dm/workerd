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
    //Offer::Vector result;

    if(params->newClient)
    {
        return;
    }

    getHistoryByType(HistoryType::Retargeting, vretageting);

    if(vretageting.size() == 0)
    {
        return;
    }

    RetargetingClear();

    //fill
    for(auto i = vretageting.begin(); i != vretageting.end(); ++i)
    {
        if(i != vretageting.begin())
            ids += ',';
        ids += (*i);
    }
/*
    minUniqueHits = 0;
    maxUniqueHits = 0;
*/
    sqlite3_snprintf(sizeof(buf), buf, cfg->retargetingOfferSqlStr.c_str(),
                     params->getUserKeyLong(),
                     ids.c_str(),
                     inf->retargeting_capacity);

    try
    {
        pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
        pStmt->Sql(buf);

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

//            off->showCount = pStmt->GetColumnInt(19);
            off->branch = EBranchL::L32;
/*
            if(minUniqueHits > off->showCount)
            {
                minUniqueHits = off->showCount;
            }

            if(maxUniqueHits < off->showCount)
            {
                maxUniqueHits = off->showCount;
            }
*/
            vRISRetargetingResult.push_back(off);
        }
        pStmt->FreeQuery();
        delete pStmt;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" Kompex::SQLiteException: "<<ex.GetString()<<std::endl;
    }

    //RISAlgorithmRetagreting(result);
}
//-------------------------------------------------------------------------------------------------------------------
void HistoryManager::RISAlgorithmRetagreting(const Offer::Vector &result)
{
    std::multiset<unsigned long long> OutPutCampaignSet;
    std::set<unsigned long long> OutPutOfferSet;

    if(result.size() == 0)
    {
        return;
    }

    for(unsigned i = minUniqueHits; i <= maxUniqueHits; i++)
    {
        //add teaser when teaser unique id and with company unique
        for(auto p = result.begin(); p != result.end(); ++p)
        {
            if(OutPutCampaignSet.count((*p)->campaign_id) == 0
                    && OutPutOfferSet.count((*p)->id_int) == 0
                    && (*p)->showCount == i)
            {
                vRISRetargetingResult.push_back((*p));
                OutPutOfferSet.insert((*p)->id_int);
                OutPutCampaignSet.insert((*p)->campaign_id);

                if(vRISRetargetingResult.size() >= inf->retargeting_capacity)
                    return;
            }
        }
    }

    //add teaser when teaser unique id
    for(auto p = result.begin(); p!=result.end(); ++p)
    {
        if(OutPutCampaignSet.count((*p)->campaign_id) < (*p)->unique_by_campaign
                && OutPutOfferSet.count((*p)->id_int) == 0)
        {
            vRISRetargetingResult.push_back((*p));
            OutPutOfferSet.insert((*p)->id_int);

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

void HistoryManager::RetargetingUpdate(const Offer::Vector &items)
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];
    int viewTime = 0;

    if(cfg->logRetargetingOfferIds && vretageting.size())
    {
        std::clog<<" retargeting offer redis:";
        for(auto i = vretageting.begin(); i != vretageting.end(); ++i)
        {
            std::clog<<" "<<*i;
        }

    }

    if(params->newClient)
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
    //pStmt->BeginTransaction();

    for(auto o = vRISRetargetingResult.begin(); o != vRISRetargetingResult.end() ; ++o)
    {
        try
        {
            sqlite3_snprintf(sizeof(buf),buf,
                             "SELECT viewTime FROM Retargeting WHERE id=%lli AND offerId=%lli;",
                             params->getUserKeyLong(), (*o)->id_int);

            pStmt->Sql(buf);
            pStmt->FetchRow();
            viewTime = pStmt->GetColumnInt64(0);
            pStmt->Reset();

            if(viewTime)
            {
                if(viewTime + cfg->retargeting_by_time_ > std::time(0))
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "UPDATE Retargeting SET uniqueHits=uniqueHits-1,showCount=showCount+1 WHERE id=%lli AND offerId=%lli;",
                                     params->getUserKeyLong(), (*o)->id_int);
                }
                else
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "DELETE FROM Retargeting WHERE id=%lli AND offerId=%lli;",
                                     params->getUserKeyLong(), (*o)->id_int);
                }
            }
            else
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Retargeting(id,offerId,uniqueHits,viewTime) VALUES(%lli,%lli,%d,%lli);",
                                 params->getUserKeyLong(), (*o)->id_int, (*o)->uniqueHits-1,std::time(0));
            }
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("HistoryManager::RetargetingUpdate select(%s) error: %s", buf, ex.GetString().c_str());
        }
    }//for

    //pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;
}

void HistoryManager::RetargetingClear()
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];

    if(params->newClient)
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
//   pStmt->BeginTransaction();

    try
    {
        sqlite3_snprintf(sizeof(buf),buf,
                         "DELETE FROM Retargeting WHERE id=%lli AND viewTime<%lli;",
                         params->getUserKeyLong(), std::time(0) - Config::Instance()->retargeting_by_time_);
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("HistoryManager::RetargetingClear(%s) error: %s", buf, ex.GetString().c_str());
    }

//    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;
}

void HistoryManager::signalHanlerRetargeting(int sigNum)
{
    std::clog<<"get signal: "<<sigNum<<std::endl;
}

