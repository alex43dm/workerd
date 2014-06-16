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

    for(auto it = items.begin()+len; it != items.end(); ++it)
    {
        if(cfg->logOutPutOfferIds)
        {
            std::clog<<" "<<(*it)->id<<" "<<(*it)->id_int<<" hits:"<<(*it)->uniqueHits<<" rate:"<<(*it)->rating;
        }

        if ((*it)->uniqueHits > 0)
        {
            if (pViewHistory->exists(key))
            {
                if (pViewHistory->zrank(key,(*it)->id_int) > 0) //if rank == -1
                {
                    if (pViewHistory->zscore(key,(*it)->id_int) > 0)
                    {
                        pViewHistory->zincrby(key, (*it)->id_int, -1);
                    }
                }
                else
                {
                    pViewHistory->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                    //pViewHistory->expire(key, Config::Instance()->views_expire_);
                }
            }
            else//if not exists
            {
                pViewHistory->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                //pViewHistory->expire(key, Config::Instance()->views_expire_);
            }
        }
    }
    return true;
}

#define OFFER_SIZE 10

bool HistoryManager::getDeprecatedOffers()
{
    if(params->newClient)
    {
        return true;
    }

    if(pViewHistory->exists(key))
    {
        std::list<std::string> offers;

        if(!pViewHistory->getRange(key, 0, -1, offers))
        {
            std::clog<<"["<<tid<<"]"<<__func__<<" pViewHistory->getRange error: "<<Module_last_error(module)<<" for key: "<<key<<std::endl;
        }
        else
        {
            size_t cmdSize = OFFER_SIZE * offers.size();
            char * cmd = new char[cmdSize];

            Kompex::SQLiteStatement *pStmt;
            pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

            try
            {
                for( auto i = offers.begin(); i != offers.end(); ++i)
                {
                    sqlite3_snprintf(cmdSize, cmd, "INSERT INTO %s(id) VALUES(%s);",
                                     tmpTable.c_str(),
                                     (*i).c_str());
                    pStmt->SqlStatement(cmd);
                }
            }
            catch(Kompex::SQLiteException &ex)
            {
                std::clog<<"["<<tid<<"]"<< typeid(this).name()<<"::"<<__func__<<" : "<<ex.GetString()<<std::endl;
            }

            delete pStmt;
            delete []cmd;
        }

        offers.clear();
    }
    else
    {
        std::clog<<"["<<tid<<"]"<<__func__<<" no history for key:"<<key<<std::endl;
    }

    if(pViewHistory->exists(key_inv))
    {
        if(!pViewHistory->getRange(key_inv, 0, -1, mtailOffers))
        {
            std::clog<<"["<<tid<<"]"<<__func__<<" error: "<<Module_last_error(module)<<" for key:"<<key<<std::endl;
        }
    }

    return true;
}

bool HistoryManager::setTailOffers(const Offer::Map &items, const Offer::Vector &toShow)
{
    bool fFound;
    for(auto it = items.begin(); it != items.end(); ++it)
    {
        fFound = false;
        for(auto i = toShow.begin(); i != toShow.end(); ++i)
        {
            if(items.count((*i)->id_int)>0)
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
            pViewHistory->zadd(key_inv, 1, (*it).first);
        }
    }
    return true;
}

void *HistoryManager::getDeprecatedOffersEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getDeprecatedOffers();
    return NULL;
}

bool HistoryManager::getDeprecatedOffersAsync()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attributes);
    //pthread_attr_setstacksize(attributes, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetDeprecatedOffersAsync, attributes, &this->getDeprecatedOffersEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(attributes);

    free(attributes);

    return true;
}

bool HistoryManager::getDeprecatedOffersAsyncWait()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_join(thrGetDeprecatedOffersAsync, 0);
//    Log::gdb("HistoryManager::getDeprecatedOffersAsyncWait return");
    return true;
}
