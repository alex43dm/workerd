#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------long term---------------------------------------
void HistoryManager::getLongTerm()
{
    if(!pLongTerm->exists(key))
    {
        return;
    }

    std::string strSH = pLongTerm->get(key);

    if(strSH.empty())
    {
        std::clog<<"["<<tid<<"]HistoryManager::"<<__func__<<" long term empty"<<std::endl;
    }
    else
    {
        std::string q = getKeywordsString(strSH);
        if (!q.empty())
        {
            lock();
            stringQuery.push_back(sphinxRequests(q,inf->range_long_term,EBranchT::T5));
            unlock();
        }
    }
}

void *HistoryManager::getLongTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getLongTerm();
    return NULL;
}
bool HistoryManager::getLongTermAsync()
{
    if(params->newClient || !inf->isLongTerm())
    {
        return true;
    }

    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetLongTermAsync, pAttr, &this->getLongTermEnv, this))
    {
        std::clog<<"["<<tid<<"]HistoryManager::"<<__func__<<" creating thread failed"<<std::endl;
    }

    pthread_attr_destroy(pAttr);

    return true;
}

bool HistoryManager::getLongTermAsyncWait()
{
    if(params->newClient || !inf->isLongTerm())
    {
        return true;
    }

    pthread_join(thrGetLongTermAsync, 0);
//    Log::gdb("HistoryManager::getLongTermAsyncWait return");
    return true;
}
