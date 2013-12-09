#ifndef GEORERIONS_H
#define GEORERIONS_H

#include "KompexSQLiteDatabase.h"

class GeoRerions
{
    public:
        GeoRerions();
        virtual ~GeoRerions();
        static bool load(Kompex::SQLiteDatabase *pdb);
    protected:
    private:
};

#endif // GEORERIONS_H
