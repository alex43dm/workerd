#include <iostream>
#include <fstream>
#include <string>
#include <strings.h>
#include <string.h>

#include "GeoRerions.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

GeoRerions::GeoRerions()
{
    //ctor
}

GeoRerions::~GeoRerions()
{
    //dtor
}

bool GeoRerions::load(Kompex::SQLiteDatabase *pdb, const std::string &fname)
{
    Kompex::SQLiteStatement *pStmt;
    pStmt = new Kompex::SQLiteStatement(pdb);
    char buf[8192], *pData;
    int sz;

    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO GeoRerions(cid,rid,rname) VALUES(");

    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    std::ifstream infile(fname);

    std::string line;
    pStmt->BeginTransaction();
    while (std::getline(infile, line))
    {
        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,"%q)", line.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("GeoRerions::load insert(%s) error: %s", buf, ex.GetString().c_str());
        }
    }
    infile.close();

    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;
    return true;
}
