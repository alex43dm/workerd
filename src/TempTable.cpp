#include <unistd.h>

#include "TempTable.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "Config.h"

#define CMD_SIZE 8192

TempTable::TempTable()
{
    tmpTableName = "tmp" + std::to_string((long long int)getpid()) + std::to_string((long long int)pthread_self());

    cmd = new char[CMD_SIZE];

    Kompex::SQLiteStatement *p;
    try
    {
        p = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
        sqlite3_snprintf(CMD_SIZE, cmd, "CREATE TABLE IF NOT EXISTS %s(id INT8 NOT NULL);",
                         tmpTableName.c_str());
        p->SqlStatement(cmd);
        /*
        sqlite3_snprintf(CMD_SIZE, cmd, "CREATE INDEX IF NOT EXISTS idx_%s_id ON %s(id);",
                         tmpTableName.c_str(), tmpTableName.c_str());
        p->SqlStatement(cmd);
        */
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" error: create tmp table: %s"<< ex.GetString()<<std::endl;
        exit(1);
    }
    delete p;
}

TempTable::~TempTable()
{
    delete []cmd;
}

bool TempTable::clearTable()
{
    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
    sqlite3_snprintf(CMD_SIZE,cmd,"DELETE FROM %s;",tmpTableName.c_str());
    try
    {
        pStmt->SqlStatement(cmd);
        pStmt->FreeQuery();
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" error: " <<ex.GetString()<<std::endl;
    }
    delete pStmt;
    return true;
}
