#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------long term---------------------------------------
void HistoryManager::getLongTerm()
{
    if(params->newClient)
    {
        return;
    }

    std::string strSH = history_archive[HistoryType::LongTerm]->get(key);

    if(strSH.empty())
    {
        Log::warn("HistoryManager::%s empty",__func__);
    }
    else if(Config::Instance()->range_long_term_ > 0)
    {
        std::string q = getKeywordsString(strSH);
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_long_term_,EBranchT::T5));
            unlock();
        }
    }

    Log::gdb("[%ld]HistoryManager::getLongTerm : done",tid);
}

void *HistoryManager::getLongTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getLongTerm();
    return NULL;
}
bool HistoryManager::getLongTermAsync()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetLongTermAsync, pAttr, &this->getLongTermEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(pAttr);

    return true;
}

bool HistoryManager::getLongTermAsyncWait()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_join(thrGetLongTermAsync, 0);
//    Log::gdb("HistoryManager::getLongTermAsyncWait return");
    return true;
}
