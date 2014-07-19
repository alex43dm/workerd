#include <boost/algorithm/string/replace.hpp>

#include "Campaign.h"
#include "Log.h"
#include "Config.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

Campaign::Campaign()
{
}
/** \brief  Конструктор

    \param id        Идентификатор рекламной кампании
*/
Campaign::Campaign(long long _id) :
    id(_id)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    sqlite3_snprintf(sizeof(buf),buf,
                     "SELECT c.guid,c.title,c.project,c.social,c.valid,c.showCoverage FROM Campaign AS c \
            WHERE c.id=%lld LIMIT 1;", _id);

    try
    {
        pStmt->Sql(buf);

        if(pStmt->FetchRow())
        {
            guid = pStmt->GetColumnString(0);
            title = pStmt->GetColumnString(1);
            project = pStmt->GetColumnString(2);
            social = pStmt->GetColumnInt(3) ? true : false;
            valid = pStmt->GetColumnInt(4) ? true : false;
            setType(pStmt->GetColumnString(5));
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("Campaign::info %s error: %s", buf, ex.GetString().c_str());
    }

    pStmt->FreeQuery();
    delete pStmt;
}

Campaign::~Campaign()
{

}

//-------------------------------------------------------------------------------------------------------
void Campaign::info(std::vector<Campaign*> &res, bool Retargeting)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;

    if(Retargeting)
    {
    sqlite3_snprintf(sizeof(buf),buf,
"SELECT c.title,c.valid,c.social,count(ofr.campaignId),c.showCoverage FROM Campaign AS c \
LEFT JOIN Offerr AS ofr ON c.id=ofr.campaignId \
WHERE c.retargeting=1 \
GROUP BY ofr.campaignId;");

    }
    else
    {
    sqlite3_snprintf(sizeof(buf),buf,
"SELECT c.title,c.valid,c.social,count(ofr.campaignId),c.showCoverage FROM Campaign AS c \
LEFT JOIN Offer AS ofr ON c.id=ofr.campaignId \
WHERE c.retargeting=0 \
GROUP BY ofr.campaignId;");
    }

    try
    {
        pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);
        pStmt->Sql(buf);

        while(pStmt->FetchRow())
        {
            Campaign *c = new Campaign();
            c->title = pStmt->GetColumnString(0);
            c->valid = pStmt->GetColumnInt(1) ? true : false;
            c->social = pStmt->GetColumnInt(2) ? true : false;
            c->offersCount = pStmt->GetColumnInt(3);
            c->setType(pStmt->GetColumnString(4));
            res.push_back(c);
        }

        pStmt->FreeQuery();
        delete pStmt;
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("Campaign::info %s error: %s", buf, ex.GetString().c_str());
    }
}

showCoverage Campaign::typeConv(const std::string &t)
{
    if( t == "all")
    {
        return showCoverage::all;
    }
    else if( t == "allowed")
    {
        return showCoverage::allowed;
    }
    else if( t == "thematic")
    {
        return showCoverage::thematic;
    }
    else
    {
        return showCoverage::all;
    }
}

void Campaign::setType(const std::string &t)
{
    if( t == "all")
    {
        type = showCoverage::all;
    }
    else if( t == "allowed")
    {
        type = showCoverage::allowed;
    }
    else if( t == "thematic")
    {
        type = showCoverage::thematic;
    }
    else
    {
        type = showCoverage::all;
    }
}

