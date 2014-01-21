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
        return history_archive[ViewHistory]->del(key);
    }

    for(auto it = items.begin(); it != items.end(); ++it)
    {
        if ((*it)->uniqueHits != -1)
        {
            if (history_archive[ViewHistory]->exists(key))
            {
                if (history_archive[ViewHistory]->zrank(key,(*it)->id_int) == -1)
                {
                    history_archive[ViewHistory]->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                }
                else//if rank == -1
                {
                    if (history_archive[ViewHistory]->zscore(key,(*it)->id_int) > 0)
                    {
                        history_archive[ViewHistory]->zincrby(key, (*it)->id_int, -1);
                    }
                }
            }
            else//if not exists
            {
                history_archive[ViewHistory]->zadd(key,(*it)->uniqueHits - 1, (*it)->id_int);
                history_archive[ViewHistory]->expire(key, Config::Instance()->views_expire_);
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

    if(!history_archive[ViewHistory]->getRange(key , 0, -1, rr))
    {
        Log::err("HistoryManager::getDeprecatedOffers error: %s", Module_last_error(module));
        return false;
    }

    return true;
}

bool HistoryManager::getDeprecatedOffers()
{
    if(!history_archive[ViewHistory]->getRange(key, tmpTable))
    {
        Log::err("HistoryManager::getDeprecatedOffers error: %s", Module_last_error(module));
        return false;
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
    pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attributes);
    //pthread_attr_setstacksize(attributes, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetDeprecatedOffersAsync, attributes, &this->getDeprecatedOffersEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(attributes);

    return true;
}

bool HistoryManager::getDeprecatedOffersAsyncWait()
{
    pthread_join(thrGetDeprecatedOffersAsync, 0);
    Log::gdb("HistoryManager::getDeprecatedOffersAsyncWait return");
    return true;
}

