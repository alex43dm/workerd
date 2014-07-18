#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

bool HistoryManager::setDeprecatedOffers(const Offer::Vector &items)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;

    if(clean)
    {
        try
        {
            pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Session WHERE id=%llu AND tail=0 AND retargeting=0;"
                             ,params->getUserKeyLong());
            pStmt->SqlStatement(buf);
            delete pStmt;
        }
        catch(Kompex::SQLiteException &ex)
        {
            std::clog<<"HistoryManager::setDeprecatedOffers delete session error: "<<ex.GetString()<<std::endl;
        }
    }

    pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
    for(auto it = items.begin(); it != items.end(); ++it)
    {
        try
        {
            sqlite3_snprintf(sizeof(buf),buf,
            "INSERT OR REPLACE INTO Session(id,offerId,uniqueHits,viewTime,retargeting) \
                SELECT %llu,%llu,ifnull(uniqueHits,%u),%llu,%u FROM Session WHERE id=%llu AND offerId=%llu;",
                    params->getUserKeyLong(), (*it)->id_int,
                    (*it)->uniqueHits, std::time(0),(*it)->retargeting,
                    params->getUserKeyLong(), (*it)->id_int);

            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            std::clog<<"HistoryManager::setDeprecatedOffers error: "<<ex.GetString()<<" request:"<<buf<<std::endl;
        }
    }
    delete pStmt;

    return true;
}
