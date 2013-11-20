#ifndef DATABASE_H
#define DATABASE_H

#include <string>

#include <stdio.h>

#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"

using namespace Kompex;

class DataBase
{
    public:
        SQLiteDatabase *pDatabase;

        DataBase();
        virtual ~DataBase();
        bool gen(int from, unsigned int len);
    protected:

    private:
        SQLiteStatement *pStmt;

        bool openDb();
};

#endif // DATABASE_H
