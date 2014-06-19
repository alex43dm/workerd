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
                    pViewHistory->expire(key, cfg->views_expire_);
                }
            }
            else//if not exists
            {
                pViewHistory->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                pViewHistory->expire(key, cfg->views_expire_);
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
        std::clog<<"["<<tid<<"]"<<__func__<<" pViewHistory->exists error: "<<Module_last_error(module)<<" no history for key: "<<key<<std::endl;
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

    if(pthread_create(&thrGetDeprecatedOffersAsync, attributes, &this->getDeprecatedOffersEnv, this))
    {
        std::clog<<"creating thread failed"<<std::endl;
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
    return true;
}
