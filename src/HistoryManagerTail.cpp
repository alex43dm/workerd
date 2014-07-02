#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

bool HistoryManager::getTailOffers()
{
    if(params->newClient)
    {
        return true;
    }

    char buf[8192];
    Kompex::SQLiteStatement *pStmt;
    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    try
    {
        sqlite3_snprintf(sizeof(buf),buf,
                         "SELECT offerId FROM Session WHERE id=%llu AND tail=1;",
                         params->getUserKeyLong());

        pStmt->Sql(buf);

        while(pStmt->FetchRow())
        {
            mtailOffers.push_back(pStmt->GetColumnInt64(0));
        }

        pStmt->FreeQuery();

        sqlite3_snprintf(sizeof(buf),buf,
                         "DELETE FROM Session WHERE id=%llu AND tail=1;",
                         params->getUserKeyLong());
        pStmt->SqlStatement(buf);
        pStmt->FreeQuery();
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"HistoryManager::getTailOffers error: "<<ex.GetString()<<" request:"<<buf<<std::endl;
    }

    delete pStmt;
    return true;
}

bool HistoryManager::setTailOffers(const Offer::Map &items, const Offer::Vector &toShow)
{
    bool fFound;
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    for(auto it = items.begin(); it != items.end(); ++it)
    {
        fFound = false;
        for(auto i = toShow.begin(); i != toShow.end(); ++i)
        {
            if((*i)->id_int == (*it).first)
            {
                fFound = true;
                break;
            }
        }

        if(fFound)
        {
            continue;
        }
        else
        {
            try
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Session(id,offerId,uniqueHits,viewTime,tail) VALUES(%llu,%llu,1,%llu,1);",
                                 params->getUserKeyLong(), (*it).first, std::time(0));

                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                std::clog<<"HistoryManager::setTailOffers error: "<<ex.GetString()<<" request:"<<buf<<std::endl;
            }
        }
    }

    pStmt->FreeQuery();
    delete pStmt;

    return true;
}

bool HistoryManager::moveUpTailOffers(Offer::Map &items, float teasersMaxRating)
{
    if(mtailOffers.size())
    {
        for(auto i = mtailOffers.begin(); i != mtailOffers.end(); ++i)
        {
            if(items[*i])
            {
                items[*i]->rating = items[*i]->rating + teasersMaxRating;
            }
        }
    }

    return true;
}

void *HistoryManager::getTailOffersEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getTailOffers();
    return NULL;
}

bool HistoryManager::getTailOffersAsync()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attributes);

    if(pthread_create(&thrGetTailOffersAsync, attributes, &this->getTailOffersEnv, this))
    {
        std::clog<<"creating thread failed"<<std::endl;
    }

    pthread_attr_destroy(attributes);

    free(attributes);

    return true;
}

bool HistoryManager::getTailOffersAsyncWait()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_join(thrGetTailOffersAsync, 0);
    return true;
}

