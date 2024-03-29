#include <signal.h>

#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "base64.h"

//----------------------------long term---------------------------------------
void HistoryManager::getLongTerm()
{
    if(!pLongTerm->exists(key))
    {
        return;
    }

    sphinx->addRequest(base64_decode(pLongTerm->get(key)),inf->range_long_term,EBranchT::T5);
    isProcessed += "l";
}

void *HistoryManager::getLongTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;

    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = signalHanlerLongTerm;
    sigaddset(&sact.sa_mask, SIGPIPE);

    if( sigaction(SIGPIPE, &sact, 0) )
    {
        std::clog<<"error set sigaction"<<std::endl;
    }

    pthread_sigmask(SIG_SETMASK, &sact.sa_mask, NULL);

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

void HistoryManager::signalHanlerLongTerm(int sigNum)
{
    std::clog<<"get signal: "<<sigNum<<std::endl;
}
