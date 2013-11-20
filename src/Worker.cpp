#include <exception>
#include <iostream>

#include "KompexSQLiteException.h"

#include "Worker.h"

Worker::Worker(Queue *q_, SQLiteDatabase *d_) :
    q(q_)
    ,db(d_)
{


}

Worker::~Worker()
{
    //dtor
}

void* Worker::run(void *data)
{
    Worker *w = (Worker*)data;

    SQLiteStatement *pStmt;
    pStmt = new SQLiteStatement(w->db);
    pStmt->Sql("SELECT id FROM Offer WHERE id > 22000");

    //Message *m;
    while(1)
    {
        //m = w->q->get();
        w->q->get();
        try
        {
            while(pStmt->FetchRow())
            {
                long id = pStmt->GetColumnInt64(0);
                //printf("%ld\n",id);
            }
        }
        catch(SQLiteException &ex)
        {
            printf("Worker DB error: %s", ex.GetString().c_str());
        }
    }
    pStmt->FreeQuery();
    return nullptr;
}
