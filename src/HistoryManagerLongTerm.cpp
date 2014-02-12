#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------long term---------------------------------------
void HistoryManager::getLongTerm()
{
    getHistoryByType(HistoryType::LongTerm, vlongTerm);

    for (auto i=vlongTerm.begin(); i != vlongTerm.end(); ++i)
    {
        std::string strSH = *i;
        if (!strSH.empty())
        {
            std::string q = getKeywordsString(strSH);
            if (!q.empty() && Config::Instance()->range_long_term_ > 0)
            {
                lock();
                stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_long_term_,EBranchT::T5));
                unlock();
            }
        }
    }
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getLongTerm : done",tid);
#endif // DEBUG
}

void *HistoryManager::getLongTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getLongTerm();
    return NULL;
}
bool HistoryManager::getLongTermAsync()
{
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
    pthread_join(thrGetLongTermAsync, 0);
//    Log::gdb("HistoryManager::getLongTermAsyncWait return");
    return true;
}
