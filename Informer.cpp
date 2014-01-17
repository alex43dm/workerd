#include "DB.h"

#include <boost/algorithm/string.hpp>

#include "Informer.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

/** Ищет данные информера по id, если не находит, то вставляет пустой элемент,
    у которого valid = false
*/
Informer::Informer(long id) :
    id(id)
{
}

Informer::Informer(long id, int capacity, const std::string &bannersCss,
                   const std::string &teasersCss, long domainId, long accountId) :
    id(id),
    teasersCss(teasersCss),
    bannersCss(bannersCss),
    capacity(capacity),
    domainId(domainId),
    accountId(accountId)
{
}

Informer::~Informer()
{
}


/** Загружает данные обо всех информерах */
bool Informer::loadAll(Kompex::SQLiteDatabase *pdb)
{
    mongo::DB db;
    std::unique_ptr<mongo::DBClientCursor> cursor = db.query("informer", mongo::Query());
    //std::set<std::string> blocked_accounts = GetBlockedAccounts();
    int skipped = 0;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *pData, *buf1;
    int sz, i = 0;
    long domainId,accountId;

    pStmt = new Kompex::SQLiteStatement(pdb);

//teasersCss?
    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO Informer(id,guid,title,bannersCss,domainId,accountId,blocked,\
             nonrelevant,valid,height,width,height_banner,width_banner,capacity) VALUES(");
    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
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

        buf1 = sqlite3_mprintf("INSERT OR IGNORE INTO Domains(name) VALUES('%q')",x.getStringField("domain"));
        try
        {
            pStmt->SqlStatement(buf1);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Domains insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);

        domainId = 0;
        try
        {
            buf1 = sqlite3_mprintf("SELECT id FROM Domains WHERE name='%q'",x.getStringField("domain"));
            pStmt->Sql(buf1);

            pStmt->FetchRow();
            domainId = pStmt->GetColumnInt64(0);
            pStmt->Reset();
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Domains insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);


        buf1 = sqlite3_mprintf("INSERT OR IGNORE INTO Accounts(name) VALUES('%q')",x.getStringField("user"));
        try
        {
            pStmt->SqlStatement(buf1);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Accounts insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);

        accountId = 0;
        try
        {
            buf1 = sqlite3_mprintf("SELECT id FROM Accounts WHERE name='%q'",x.getStringField("user"));
            pStmt->Sql(buf1);

            pStmt->FetchRow();
            accountId = pStmt->GetColumnInt64(0);
            pStmt->Reset();
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Accounts insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,
                         "%lld,'%q','%q','%q',%d,%d,0,'%q',1,%d,%d,%d,%d,%d)",
                         x.getField("guid_int").numberLong(),
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("css"),
                         domainId,
                         accountId,
                         x.getStringField("nonRelevant"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getIntField("height_banner"),
                         x.getIntField("width_banner"),
                         capacity
                         );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::_loadFromQuery insert error: %s", ex.GetString().c_str());
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


bool Informer::update(Kompex::SQLiteDatabase *pdb, std::string aInformerId)
{
    mongo::DB db;
    std::unique_ptr<mongo::DBClientCursor> cursor = db.query("informer", QUERY("guid" << aInformerId));
    int skipped = 0;
    long long long_id = 0;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *buf1;
    int i = 0;
    long domainId,accountId;

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();

    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
        boost::to_lower(id);
        if (id.empty())
        {
            skipped++;
            continue;
        }

        long_id = x.getField("guid_int").numberLong();

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

        buf1 = sqlite3_mprintf("INSERT OR IGNORE INTO Domains(name) VALUES('%q')",x.getStringField("domain"));
        try
        {
            pStmt->SqlStatement(buf1);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Domains insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);

        domainId = 0;
        try
        {
            buf1 = sqlite3_mprintf("SELECT id FROM Domains WHERE name='%q'",x.getStringField("domain"));
            pStmt->Sql(buf1);

            pStmt->FetchRow();
            domainId = pStmt->GetColumnInt64(0);
            pStmt->Reset();
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Domains insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);


        buf1 = sqlite3_mprintf("INSERT OR IGNORE INTO Accounts(name) VALUES('%q')",x.getStringField("user"));
        try
        {
            pStmt->SqlStatement(buf1);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Accounts insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);

        accountId = 0;
        try
        {
            buf1 = sqlite3_mprintf("SELECT id FROM Accounts WHERE name='%q'",x.getStringField("user"));
            pStmt->Sql(buf1);

            pStmt->FetchRow();
            accountId = pStmt->GetColumnInt64(0);
            pStmt->Reset();
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::Accounts insert(%s) error: %s", buf1, ex.GetString().c_str());
        }
        sqlite3_free((void*)buf1);

        bzero(buf,sizeof(buf));

    snprintf(buf,sizeof(buf),"INSERT INTO ) VALUES(");

        sqlite3_snprintf(sizeof(buf),buf,
                         "UPDATE Informer SET\
                         title='%q',\
                         bannersCss='%q',\
                         domainId=%d,\
                         accountId=%d,\
                         blocked=0,\
                         nonrelevant='%q',\
                         valid=1,\
                         height=%d,\
                         width=%d,\
                         height_banner=%d,\
                         width_banner=%d,\
                         capacity=%d \
                         WHERE id=%lld;",
                         x.getStringField("title"),
                         x.getStringField("css"),
                         domainId,
                         accountId,
                         x.getStringField("nonRelevant"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getIntField("height_banner"),
                         x.getIntField("width_banner"),
                         capacity,
                         long_id
                         );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::_loadFromQuery insert error: %s", ex.GetString().c_str());
            skipped++;
        }
        // Тематические категории
        //LoadCategoriesByDomain(data->categories, x.getStringField("domain"));

        i++;
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("updated informer id %lld", long_id);
    if (skipped)
        Log::warn("Informers with empty id skipped: %d", skipped);
    return true;
}
