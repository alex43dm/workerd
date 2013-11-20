#ifndef WORKER_H
#define WORKER_H

#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"

#include <Queue.h>
#include <DataBase.h>

class Worker
{
    public:
        Worker(Queue *, SQLiteDatabase *);
        virtual ~Worker();
        static void* run(void*);
    protected:
        Queue *q;
        SQLiteDatabase *db;
    private:

};

#endif // WORKER_H
