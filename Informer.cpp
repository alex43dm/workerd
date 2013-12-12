#include "DB.h"
#include <map>
#include <boost/algorithm/string.hpp>
#include "Informer.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

using namespace std;
using namespace mongo;

/** Ищет данные информера по id, если не находит, то вставляет пустой элемент,
    у которого valid = false
*/
Informer::Informer(long id) :
    id(id)
{
}

Informer::Informer(long id, int capacity, const std::string &bannersCss, const std::string &teasersCss) :
    id(id),
    teasersCss(teasersCss),
    bannersCss(bannersCss),
    capacity(capacity)
{
}

Informer::~Informer()
{
}


/** Загружает данные обо всех информерах */
bool Informer::loadAll(Kompex::SQLiteDatabase *pdb)
{
    mongo::DB db;
    unique_ptr<mongo::DBClientCursor> cursor = db.query("informer", mongo::Query());
    //std::set<std::string> blocked_accounts = GetBlockedAccounts();
    int skipped = 0;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *pData;
    int sz, i = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);

//teasersCss?
    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO Informer(id,guid,title,bannersCss,domain,user,blocked,\
             nonrelevant,valid,height,width,height_banner,width_banner,capacity) VALUES(");
    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        string id = x.getStringField("guid");
        boost::to_lower(id);
        if (id.empty())
        {
            skipped++;
            continue;
        }
        int capacity = 0;
        mongo::BSONElement capacity_element =
            x.getFieldDotted("admaker.Main.itemsNumber");
        switch (capacity_element.type())
        {
        case mongo::NumberInt:
            capacity = capacity_element.numberInt();
            break;
        case mongo::String:
            capacity =
                boost::lexical_cast<int>(capacity_element.str());
            break;
        default:
            capacity = 0;
        }

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,
                         "%lld,'%q','%q','%q','%q','%q',0,'%q',1,%d,%d,%d,%d,%d)",
                         x.getField("guid_int").numberLong(),
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("css"),
                         x.getStringField("domain"),
                         x.getStringField("user"),
                         x.getStringField("nonRelevant"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getIntField("height_banner"),
                         x.getIntField("width_banner"),
                         capacity);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::_loadFromQuery insert(%s) error: %s", buf, ex.GetString().c_str());
            skipped++;
        }
        // Тематические категории
        //LoadCategoriesByDomain(data->categories, x.getStringField("domain"));

        i++;
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Loaded %d informers", i);
    if (skipped)
        Log::warn("Informers with empty id skipped: %d", skipped);
    return true;
}

bool Informer::operator==(const Informer &other) const
{
    return this->id == other.id;
}

bool Informer::operator<(const Informer &other) const
{
    return this->id < other.id;
}
