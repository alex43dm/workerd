#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------Retargeting---------------------------------------
void HistoryManager::getRetargeting()
{
    getHistoryByType(HistoryType::Retargeting, vretageting);
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getRetargeting : done",tid);
#endif // DEBUG
}

void *HistoryManager::getRetargetingEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;
    h->getRetargeting();
    return NULL;
}
bool HistoryManager::getRetargetingAsync()
{
    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetRetargetingAsync, pAttr, &this->getRetargetingEnv, this))
    {
        Log::err("creating thread failed");
    }

    pthread_attr_destroy(pAttr);

    return true;
}

std::string HistoryManager::getRetargetingAsyncWait()
{
    std::string ret;
    pthread_join(thrGetRetargetingAsync, 0);
    for(auto i = vretageting.begin(); i != vretageting.end(); ++i)
    {
        if(i != vretageting.begin())
            ret += ',';
        ret += (*i);
    }
#ifdef DEBUG
    Log::info("[%ld]HistoryManager::getRetargetingAsyncWait return",tid);
#endif // DEBUG
    return ret;
}
