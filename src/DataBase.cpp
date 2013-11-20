#include <iostream>

#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdlib.h>

#include "Log.h"
#include "DataBase.h"
#include "KompexSQLiteException.h"

#define INSERTSTATMENT "INSERT INTO Offer (id) VALUES (%lu)"

DataBase::DataBase() :
    pStmt(nullptr)
{
    openDb();
}

DataBase::~DataBase()
{
    //dtor
}

bool DataBase::openDb()
{
    try
    {
        pDatabase = new SQLiteDatabase(":memory:",
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
    }
    catch(SQLiteException &exception)
    {
        Log::err("DB error: %s", exception.GetString().c_str());
        exit(1);
    }

    if(pStmt)
    {
        delete pStmt;
    }
    pStmt = new SQLiteStatement(pDatabase);

    try
    {
        pStmt->SqlStatement("CREATE TABLE IF NOT EXISTS Offer \
                        (                                           \
                        id INTEGER NOT NULL,            \
                        title VARCHAR(1024),                 \
                        price REAL,               \
                        description TEXT,                           \
                        url VARCHAR(2048),                   \
                        image_url VARCHAR(2048),                          \
                        campaign_id INTEGER,                             \
                        valid INTEGER                          \
                        )");
    }
    catch(SQLiteException &exception)
    {
        Log::err("DB error: %s", exception.GetString().c_str());
    }
    return true;
}

bool DataBase::gen(int from, unsigned int len)
{
    char buf[2048];

    try
    {
        pStmt->BeginTransaction();

        for(unsigned long i = 0; i < len; i++)
        {
            bzero(buf,sizeof(buf));
            snprintf(buf,sizeof(buf),INSERTSTATMENT, from + i);
            pStmt->SqlStatement(buf);
        }

        pStmt->CommitTransaction();
    }
    catch(SQLiteException &ex)
    {
        printf("DataBase DB error: %s", ex.GetString().c_str());
    }
    return true;
}
