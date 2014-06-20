#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

bool HistoryManager::clearDeprecatedOffers()
{
    return pViewHistory->del(key);
}

bool HistoryManager::setDeprecatedOffers(const Offer::Vector &items, unsigned len)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;
    int viewTime = 0;

    if(cfg->logOutPutOfferIds)
    {
        std::clog<<" OutPutOfferIds: ";
    }

    if(clean)
    {
        if(cfg->logOutPutOfferIds)
        {
            std::clog<<"[clean]";
        }
        pViewHistory->del(key);
    }

    if(cfg->logOutPutOfferIds)
    {
        std::clog<<", ids:";
    }

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    for(auto it = items.begin()+len; it != items.end(); ++it)
    {
        if(cfg->logOutPutOfferIds)
        {
            std::clog<<" "<<(*it)->id<<" "<<(*it)->id_int<<" hits:"<<(*it)->uniqueHits<<" rate:"<<(*it)->rating;
        }

        try
        {
            sqlite3_snprintf(sizeof(buf),buf,
                             "SELECT viewTime FROM Session WHERE id=%lli AND offerId=%lli;",
                             params->getUserKeyLong(), (*it)->id_int);

            pStmt->Sql(buf);
            pStmt->FetchRow();
            viewTime = pStmt->GetColumnInt64(0);
            pStmt->Reset();

            if(viewTime)
            {
                if(viewTime + cfg->retargeting_by_time_ > std::time(0))
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "UPDATE Session SET uniqueHits=uniqueHits-1 WHERE id=%lli AND offerId=%lli;",
                                     params->getUserKeyLong(), (*it)->id_int);
                }
                else
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "DELETE FROM Session WHERE id=%lli AND offerId=%lli;",
                                     params->getUserKeyLong(), (*it)->id_int);
                }
            }
            else
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Session(id,offerId,uniqueHits,viewTime) VALUES(%lli,%lli,%d,%lli);",
                                 params->getUserKeyLong(), (*it)->id_int, (*it)->uniqueHits-1,std::time(0));
            }
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("HistoryManager::setDeprecatedOffers select(%s) error: %s", buf, ex.GetString().c_str());
        }
    }
    return true;
}
