#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------short term---------------------------------------
void HistoryManager::getShortTerm()
{
    if(!pShortTerm->exists(key))
    {
        return;
    }

    std::string strSH = pShortTerm->get(key);

    if(strSH.empty())
    {
        std::clog<<"["<<tid<<"]HistoryManager::"<<__func__<<" sort term empty"<<std::endl;
    }
    else
    {
        std::string q = getKeywordsString(strSH);
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,inf->range_short_term,EBranchT::T3));
            unlock();
        }
    }
}

void *HistoryManager::getShortTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getShortTerm();
    return NULL;
}

bool HistoryManager::getShortTermAsync()
{
    if(params->newClient || !inf->isShortTerm())
    {
        return true;
    }

    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetShortTermAsync, pAttr, &this->getShortTermEnv, this))
    {
        std::clog<<"["<<tid<<"]HistoryManager::"<<__func__<<" creating thread failed"<<std::endl;
    }

    pthread_attr_destroy(pAttr);

    return true;
}

bool HistoryManager::getShortTermAsyncWait()
{
    if(params->newClient || !inf->isShortTerm())
    {
        return true;
    }

    pthread_join(thrGetShortTermAsync, 0);
//    Log::gdb("HistoryManager::getShortTermAsyncWait return");
    return true;
}
