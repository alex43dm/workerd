#include <exception>
#include <iostream>
#include "Worker.h"

Worker::Worker(Queue *q_, dbDatabase *d_) :
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

    w->db->attach();
/*
    dbDatabase *db = new dbDatabase(dbDatabase::dbConcurrentRead);
    db->open("/tmp/fastdb");
*/
    dbCursor<Offer> cOffers;

    dbQuery qOffers;
    qOffers.reset();
    qOffers.And("id=").add(22);

    //Message *m;
    while(1)
    {
        //m = w->q->get();
        w->q->get();
        try
        {
            if (cOffers.select(qOffers) > 0)
            {
                do
                {
                    printf("%d\n", cOffers->id);
                }
                while (cOffers.next());
            }
        }
        catch(std::exception &e)
        {
            std::cerr << "exception caught: " << e.what() << '\n';
        }
    }

    return nullptr;
}
