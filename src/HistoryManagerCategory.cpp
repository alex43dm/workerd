#include <signal.h>

#include "HistoryManager.h"
#include "Config.h"
#include "Log.h"

//----------------------------Category---------------------------------------
void HistoryManager::getCategory()
{
    if(params->newClient)
    {
        return;
    }

    if(!pCategory->exists(key))
    {
        return;
    }

    std::list<unsigned> cats;

    if(pCategory->zrange(key,cats))
    {
        std::string allCats;
        for(auto i = cats.begin(); i != cats.end(); ++i)
        {
            if(i!=cats.begin())
            {
                allCats += "|"+cfg->Categories[*i];
            }
            else
            {
                allCats = cfg->Categories[*i];
            }
        }
        sphinx->addRequest(allCats,1.0,EBranchT::T3);
        isProcessed += "t";
    }
}

void *HistoryManager::getCategoryEnv(void *data)
{
    HistoryManager *h = (HistoryManager*)data;

    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = signalHanlerCategory;
    sigaddset(&sact.sa_mask, SIGPIPE);

    if( sigaction(SIGPIPE, &sact, 0) )
    {
        std::clog<<"error set sigaction"<<std::endl;
    }

    pthread_sigmask(SIG_SETMASK, &sact.sa_mask, NULL);

    h->getCategory();
    return NULL;
}

bool HistoryManager::getCategoryAsync()
{
    if(params->newClient)
    {
        return true;
    }

    pthread_attr_t attributes, *pAttr = &attributes;
    pthread_attr_init(pAttr);
    //pthread_attr_setstacksize(pAttr, THREAD_STACK_SIZE);

    if(pthread_create(&thrGetRetargetingAsync, pAttr, &this->getRetargetingEnv, this))
    {
        std::clog<<"creating thread failed"<<std::endl;
    }

    pthread_attr_destroy(pAttr);

    return true;
}

void HistoryManager::getCategoryAsyncWait()
{

    if(params->newClient)
    {
        return;
    }

    pthread_join(thrGetCategoryAsync, 0);
    return;
}

void HistoryManager::signalHanlerCategory(int sigNum)
{
    std::clog<<__func__<<"Category get signal: "<<sigNum<<std::endl;
}

