#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

bool HistoryManager::clearDeprecatedOffers()
{
    return history_archive[ViewHistory]->del(key);
}

bool HistoryManager::setDeprecatedOffers(const Offer::Vector &items)
{
    if(clean)
    {
        history_archive[ViewHistory]->del(key);
    }

    for(auto it = items.begin(); it != items.end(); ++it)
    {
        if ((*it)->uniqueHits > 0)
        {
            if (history_archive[ViewHistory]->exists(key))
            {
                if (history_archive[ViewHistory]->zrank(key,(*it)->id_int) > 0) //if rank == -1
                {
                    if (history_archive[ViewHistory]->zscore(key,(*it)->id_int) > 0)
                    {
                        history_archive[ViewHistory]->zincrby(key, (*it)->id_int, -1);
                    }
                }
                else
                {
                    history_archive[ViewHistory]->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                    //history_archive[ViewHistory]->expire(key, Config::Instance()->views_expire_);
                }
            }
            else//if not exists
            {
                history_archive[ViewHistory]->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                //history_archive[ViewHistory]->expire(key, Config::Instance()->views_expire_);
            }
        }
    }
    return true;
}

/** \brief Получение идентификаторов РП от индекса lucene.
 *
 * Возвращает список пар (идентификатор,вес), отсортированный по убыванию веса.
 *
 * В этом методе происходит обращение к redis за историей пользователя и отбор идентификаторов РП из индекса CLucene.
 */
bool HistoryManager::getDeprecatedOffers(std::string &rr)
{
    clean = false;

    if(!history_archive[ViewHistory]->exists(key))
    {
        //Log::err("%s::%s error: %s", typeid(this),__func__,Module_last_error(module));
        return false;
    }

    if(!history_archive[ViewHistory]->getRange(key , 0, -1, rr))
    {
        Log::err("HistoryManager::getDeprecatedOffers error: %s", Module_last_error(module));
        return false;
    }

    return true;
}

bool HistoryManager::getDeprecatedOffers()
{
    if(history_archive[ViewHistory]->exists(key))
    {
        if(!history_archive[ViewHistory]->getRange(key, tmpTable))
        {
            Log::err("[%ld]%s::%s error: %s for key: %s", tid, typeid(this).name(), __func__, Module_last_error(module), key.c_str());
        }
    }
    else
    {
        Log::warn("[%ld]%s::%s: no history for key: %s",tid, typeid(this).name(),__func__, key.c_str());
    }

    if(history_archive[ViewHistory]->exists(key_inv))
    {
        if(!history_archive[ViewHistory]->getRange(key_inv, 0, -1, mtailOffers))
        {
            Log::err("[%ld]%s::%s error: %s", tid, typeid(this).name(), __func__,Module_last_error(module));
        }
    }

    Log::gdb("[%ld]%s::%s: done",tid, typeid(this).name(), __func__);
    return true;
}

bool HistoryManager::setTailOffers(const Offer::Map &items)
{
    for(auto it = items.begin(); it != items.end(); ++it)
    {
        history_archive[ViewHistory]->zadd(key_inv, 1, (*it).first);
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
