#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

//----------------------------Retargeting---------------------------------------
void HistoryManager::getRetargeting()
{
    getHistoryByType(HistoryType::Retargeting, vretageting);
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

std::string HistoryManager::getRetargetingAsyncWait()
{
    std::string ret;
    pthread_join(thrGetRetargetingAsync, 0);
    for(auto i = vretageting.begin(); i != vretageting.end(); ++i)
    {
        if(i != vretageting.begin())
            ret += ',';
        ret += (*i);
    }
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getRetargetingAsyncWait return",tid);
#endif // DEBUG
    return ret;
}

void HistoryManager::RetargetingUpdate(const Params *pa, const Offer::Vector &v, unsigned len)
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
                             pa->getUserKeyLong(), v[i]->id_int);

            pStmt->Sql(buf);
            pStmt->FetchRow();
            viewTime = pStmt->GetColumnInt64(0);
            pStmt->Reset();

            if(viewTime)
            {
                if(viewTime+24*3600 > std::time(0))
                {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "UPDATE Retargeting SET uniqueHits=uniqueHits-1 WHERE id=%lli AND offerId=%lli;",
                                 pa->getUserKeyLong(), v[i]->id_int);
                }
                else
                {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "DELETE FROM Retargeting WHERE id=%lli AND offerId=%lli;",
                                 pa->getUserKeyLong(), v[i]->id_int);
                }
            }
            else
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Retargeting(id,offerId,uniqueHits,viewTime) VALUES(%lli,%lli,%d,%lli);",
                                 pa->getUserKeyLong(), v[i]->id_int, v[i]->uniqueHits,std::time(0));
            }
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("HistoryManager::RetargetingUpdate select(%s) error: %s", buf, ex.GetString().c_str());
        }
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

#ifdef DEBUG
    Log::info("[%ld]HistoryManager::RetargetingUpdate return",tid);
#endif // DEBUG
}
