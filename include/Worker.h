#ifndef WORKER_H
#define WORKER_H

#include <Queue.h>
#include <MainDb.h>

class Worker
{
    public:
        Worker(Queue *, dbDatabase *);
        virtual ~Worker();
        static void* run(void*);
    protected:
        Queue *q;
        dbDatabase *db;
    private:
};

#endif // WORKER_H
