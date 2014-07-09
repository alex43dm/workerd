#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

bool HistoryManager::setDeprecatedOffers(const Offer::Vector &items)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;
    int viewTime = 0;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    if(clean)
    {
        try
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Session WHERE id=%llu AND tail=0 AND retargeting=0;"
                             ,params->getUserKeyLong());
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            std::clog<<"HistoryManager::setDeprecatedOffers delete session error: "<<ex.GetString()<<std::endl;
        }
    }

    for(auto it = items.begin(); it != items.end(); ++it)
    {
        try
        {
            sqlite3_snprintf(sizeof(buf),buf,
                             "SELECT viewTime FROM Session WHERE id=%llu AND offerId=%llu AND tail=0;",
                             params->getUserKeyLong(), (*it)->id_int);

            pStmt->Sql(buf);
            pStmt->FetchRow();
            viewTime = pStmt->GetColumnInt64(0);
            pStmt->Reset();

            if(viewTime)
            {
                if(viewTime + cfg->views_expire_ > std::time(0))
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "UPDATE Session SET uniqueHits=uniqueHits-1 WHERE id=%llu AND offerId=%llu;",
                                     params->getUserKeyLong(), (*it)->id_int);
                }
                else
                {
                    sqlite3_snprintf(sizeof(buf),buf,
                                     "DELETE FROM Session WHERE id=%llu AND offerId=%llu AND retargeting=0;",
                                     params->getUserKeyLong(), (*it)->id_int);
                }
            }
            else
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Session(id,offerId,uniqueHits,viewTime,tail,retargeting) VALUES(%llu,%llu,%d,%llu,0,%u);",
                                 params->getUserKeyLong(), (*it)->id_int, (*it)->uniqueHits-1,std::time(0),(*it)->retargeting);
            }
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            std::clog<<"HistoryManager::setDeprecatedOffers error: "<<ex.GetString()<<" request:"<<buf<<std::endl;
        }
    }

    pStmt->FreeQuery();
    delete pStmt;

    return true;
}
