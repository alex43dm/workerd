#include <iostream>
#include <fstream>
#include <string>
#include <strings.h>
#include <string.h>

#include <vector>
#include <boost/algorithm/string.hpp>

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
    snprintf(buf,sizeof(buf),"INSERT INTO GeoLiteCity(locId,country,region,city,postalCode,latitude,longitude,metroCode,areaCode) VALUES(");

    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    std::ifstream infile(fname);

    std::string line;
    pStmt->BeginTransaction();
    while (std::getline(infile, line))
    {
        std::vector<std::string> val;
        boost::split(val, line, boost::is_any_of(","));

        if(val.size() < 9)
        {
            Log::warn("%s",line.c_str());
            continue;
        }

/*        if(x[1]!="UA" && x[1]!="RU")
        {
            continue;
        }
*/
        for(unsigned i=0; i < val.size(); i++)
        {
            boost::replace_first(val[i], "\"", "");
            boost::replace_last(val[i], "\"", "");
            if(i>4 && !val[i].size())
            {
                val[i]="NULL";
            }
        }

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,"%s,'%q','%q','%q','%q',%s,%s,%s,%s);",
                         val[0].c_str(),
                         val[1].c_str(),
                         val[2].c_str(),
                         val[3].c_str(),
                         val[4].c_str(),
                         val[5].c_str(),
                         val[6].c_str(),
                         val[7].c_str(),
                         val[8].c_str()
                         );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("GeoRerions::load insert: %s error: %s", buf, ex.GetString().c_str());
        }
    }
    infile.close();

    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("GeoLiteCity loaded");

    return true;
}

