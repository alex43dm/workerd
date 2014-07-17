#include <signal.h>

#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"
#include "base64.h"

//----------------------------short term---------------------------------------
void HistoryManager::getShortTerm()
{
    if(!pShortTerm->exists(key.c_str()))
    {
        return;
    }

    if(pLongTerm->get(key.c_str()))
    {
        sphinx->addRequest(base64_decode(std::string(pLongTerm->getData())),inf->range_short_term,EBranchT::T4);
    }
}

void *HistoryManager::getShortTermEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;

    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = signalHanlerShortTerm;
    sigaddset(&sact.sa_mask, SIGPIPE);

    if( sigaction(SIGPIPE, &sact, 0) )
    {
        std::clog<<"error set sigaction"<<std::endl;
    }

    pthread_sigmask(SIG_SETMASK, &sact.sa_mask, NULL);

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

void HistoryManager::signalHanlerShortTerm(int sigNum)
{
    std::clog<<"get signal: "<<sigNum<<std::endl;
}
