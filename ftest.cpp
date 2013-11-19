//-< TESTTL.CPP >----------------------------------------------------*--------*
// FastDB                    Version 1.0         (c) 1999  GARRET    *     ?  *
// (Main Memory Database Management System)                          *   /\|  *
//                                                                   *  /  \  *
//                          Created:     23-Aug-2010  K.A. Knizhnik  * / [] \ *
//                          Last update: 23-Aug-2010  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Transaction logging demo
//-------------------------------------------------------------------*--------*

#include "fastdb.h"
#include <stdio.h>

USE_FASTDB_NAMESPACE


class Offer
{
public:
    	int8 id;
    	int8 type;
	const char *name;
	Offer(int8 id_, int8 type_, const char *name_):
		id(id_),
		type(type_),
		name(name_){}
	TYPE_DESCRIPTOR((KEY(id, AUTOINCREMENT|INDEXED), FIELD(type), FIELD(name)));
};

REGISTER(Offer);

int main(int argc, char* argv[])
{
    dbDatabase db;
    Offer of;
    size_t n;    
        
    // Open database and transaction logger
    db.open(_T("testtl"));

    // Add one developer in his team...
    of.name = "Peter Green";
    of.id = 39;
    of.type = 1;
    insert(of);

    db.close();
}



