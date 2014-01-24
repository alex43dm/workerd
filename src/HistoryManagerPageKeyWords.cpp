#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
//----------------------------page keywords---------------------------------------
void *HistoryManager::getPageKeywordsEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getPageKeywordsHistory();
    return NULL;
}


void HistoryManager::getPageKeywordsHistory()
{
    getHistoryByType(HistoryType::PageKeywords, vkeywords);

    for (auto i=vkeywords.begin(); i != vkeywords.end(); ++i)
    {
        std::string strSH = *i;
        if (!strSH.empty())
        {
            std::string q = params->getContextKeywordsString(strSH);
            if (!q.empty())
            {
                lock();
                stringQuery.push_back(sphinxRequests(q,Config::Instance()->range_context_term_,EBranchT::T4));
                unlock();
            }
        }
    }
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getPageKeywordsHistory : done",tid);
#endif // DEBUG
}



bool HistoryManager::getPageKeywordsAsync()
{
    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetPageKeywordsAsync, pAttr, &this->getPageKeywordsEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(pAttr);

    return true;
}

bool HistoryManager::getPageKeywordsAsyncWait()
{
    pthread_join(thrGetPageKeywordsAsync, 0);
//    Log::gdb("HistoryManager::getPageKeywordsAsyncWait return");
    return true;
}



void HistoryManager::updatePageKeywordsHistory(const std::string & query)
{
    if(query.empty() && !updateContext)
        return;

    history_archive[PageKeywords]->zadd(key, currentDateToInt(), query);
    history_archive[PageKeywords]->expire(key, Config::Instance()->context_expire_);
    if (history_archive[PageKeywords]->zcount(key) >= 3)
    {
        history_archive[PageKeywords]->zremrangebyrank(key, 0, 0);
    }
}
