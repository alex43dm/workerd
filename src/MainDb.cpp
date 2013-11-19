#include "MainDb.h"

//REGISTER(Campaign);
REGISTER(Offer);
//REGISTER(Informer);

MainDb::MainDb(const std::string &path)
{
    db = new dbDatabase(dbDatabase::dbAllAccess);
    db->open(path.c_str());
    //db->setConcurrency(20);
}

MainDb::~MainDb()
{
    db->close();
    delete db;
}

bool MainDb::gen(int len)
{
    Offer *of;

    for(auto i = 0; i < len; i++)
    {
        of = new Offer();
        of->id = i;//(const char*)malloc(1024*4);
        //snprintf((char*)of->id, 1024*4, "%d", i);
        db->insert(*of);
    }

    db->commit();

    return true;
}

bool MainDb::get()
{
    dbCursor<Offer> cOffers;
    dbQuery qOffers;

    //qOffers.reset();
    //qOffers.And("type>").add(1000);

    if (cOffers.select() > 0)
    {
	    do
	    {
            printf("%d\n", cOffers->id);
	    }
        while (cOffers.next());
    }

    return true;
}
