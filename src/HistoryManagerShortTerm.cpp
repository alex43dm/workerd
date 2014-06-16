#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------short term---------------------------------------
void HistoryManager::getShortTerm()
{
    if(params->newClient && !pShortTerm->exists(key))
    {
        return;
    }

    std::string strSH = pShortTerm->get(key);

    if(strSH.empty())
    {
        Log::warn("HistoryManager::%s empty",__func__);
    }
    else if(Config::Instance()->range_short_term_ > 0)
    {
        std::string q = getKeywordsString(strSH);
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_short_term_,EBranchT::T3));
            unlock();
        }
    }

    Log::gdb("[%ld]HistoryManager::getShortTerm : done",tid);
}

void *HistoryManager::getShortTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getShortTerm();
    return NULL;
}

bool HistoryManager::getShortTermAsync()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetShortTermAsync, pAttr, &this->getShortTermEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(pAttr);

    return true;
}

bool HistoryManager::getShortTermAsyncWait()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_join(thrGetShortTermAsync, 0);
//    Log::gdb("HistoryManager::getShortTermAsyncWait return");
    return true;
}

void HistoryManager::updateShortHistory(const std::string & query)
{
    if(query.empty() && !updateShort)
        return;
    /*
        history_archive[ShortTerm]->zadd(key, currentDateToInt(), query);
        history_archive[ShortTerm]->expire(key, Config::Instance()->shortterm_expire_);
        if (history_archive[ShortTerm]->zcount(key) >= 3)
        {
            history_archive[ShortTerm]->zremrangebyrank(key, 0, 0);
        }*/
}
