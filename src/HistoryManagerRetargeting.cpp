#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------Retargeting---------------------------------------
void HistoryManager::getRetargeting()
{
    getHistoryByType(HistoryType::Retargeting, vretageting);
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

bool HistoryManager::getRetargetingAsyncWait()
{
    pthread_join(thrGetRetargetingAsync, 0);
    Log::gdb("HistoryManager::getRetargetingAsyncWait return");
    return true;
}
