#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------short term---------------------------------------
void HistoryManager::getShortTerm()
{
    getHistoryByType(HistoryType::ShortTerm, vshortTerm);

    for (auto i=vshortTerm.begin(); i != vshortTerm.end(); ++i)
    {
        std::string strSH = *i;
        if (!strSH.empty())
        {
            std::string q = Params::getKeywordsString(strSH);
            if (!q.empty())
            {
                lock();
                stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_short_term_,EBranchT::T3));
                unlock();
            }
        }
    }
    Log::info("[%ld]HistoryManager::getShortTerm : done",tid);
}

void *HistoryManager::getShortTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getShortTerm();
    return NULL;
}

bool HistoryManager::getShortTermAsync()
{
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
    pthread_join(thrGetShortTermAsync, 0);
//    Log::gdb("HistoryManager::getShortTermAsyncWait return");
    return true;
}

void HistoryManager::updateShortHistory(const std::string & query)
{
    if(query.empty() && !updateShort)
        return;

    history_archive[ShortTerm]->zadd(key, currentDateToInt(), query);
    history_archive[ShortTerm]->expire(key, Config::Instance()->shortterm_expire_);
    if (history_archive[ShortTerm]->zcount(key) >= 3)
    {
        history_archive[ShortTerm]->zremrangebyrank(key, 0, 0);
    }
}
