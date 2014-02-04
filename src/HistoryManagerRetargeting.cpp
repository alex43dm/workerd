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

    RetargetingClear();

    getHistoryByType(HistoryType::Retargeting, vretageting);
    //fill
    for(auto i = vretageting.begin(); i != vretageting.end(); ++i)
    {
        if(i != vretageting.begin())
            ids += ',';
        ids += (*i);
    }

    sqlite3_snprintf(sizeof(buf), buf, RetargetingOfferStr.c_str(), params->getUserKeyLong(), ids.c_str());

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

            vretg->push_back(off);
        }
        pStmt->FreeQuery();
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
    }

    delete pStmt;
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getRetargeting : done",tid);
#endif // DEBUG
}

void *HistoryManager::getRetargetingEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getRetargeting();
    return NULL;
}

bool HistoryManager::getRetargetingAsync()
{
    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetRetargetingAsync, pAttr, &this->getRetargetingEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(pAttr);

    return true;
}

void HistoryManager::getRetargetingAsyncWait()
{

    pthread_join(thrGetRetargetingAsync, 0);
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getRetargetingAsyncWait return",tid);
#endif // DEBUG
    return;
}

void HistoryManager::RetargetingUpdate(const Offer::Vector &v, unsigned len)
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];
    int viewTime = 0;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
    pStmt->BeginTransaction();

    for(unsigned i = 0; i < len && i < v.size(); i++)
    {
        try
        {
            sqlite3_snprintf(sizeof(buf),buf,
                             "SELECT viewTime FROM Retargeting WHERE id=%lli AND offerId=%lli;",
                             params->getUserKeyLong(), v[i]->id_int);

            pStmt->Sql(buf);
            pStmt->FetchRow();
            viewTime = pStmt->GetColumnInt64(0);
            pStmt->Reset();

            if(viewTime)
            {
                if(viewTime + Config::Instance()->retargeting_by_time_ > std::time(0))
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "UPDATE Retargeting SET uniqueHits=uniqueHits-1 WHERE id=%lli AND offerId=%lli;",
                                     params->getUserKeyLong(), v[i]->id_int);
                }
                else
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "DELETE FROM Retargeting WHERE id=%lli AND offerId=%lli;",
                                     params->getUserKeyLong(), v[i]->id_int);
                }
            }
            else
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Retargeting(id,offerId,uniqueHits,viewTime) VALUES(%lli,%lli,%d,%lli);",
                                 params->getUserKeyLong(), v[i]->id_int, v[i]->uniqueHits,std::time(0));
            }
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("HistoryManager::RetargetingUpdate select(%s) error: %s", buf, ex.GetString().c_str());
        }
    }//for

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

#ifdef DEBUG
    Log::info("[%ld]HistoryManager::RetargetingUpdate return",tid);
#endif // DEBUG
}

void HistoryManager::RetargetingClear()
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
    pStmt->BeginTransaction();

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

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

#ifdef DEBUG
    Log::info("[%ld]HistoryManager::RetargetingClear return",tid);
#endif // DEBUG
}
