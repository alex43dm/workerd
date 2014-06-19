#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

bool HistoryManager::getTailOffers()
{
    if(params->newClient)
    {
        return true;
    }

    if(pViewHistory->exists(key_inv))
    {
        if(!pViewHistory->getRange(key_inv, 0, -1, mtailOffers))
        {
            std::clog<<"["<<tid<<"]"<<__func__<<" error: "<<Module_last_error(cfg->module)<<" for key:"<<key<<std::endl;
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

        mtailOffers.clear();
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
