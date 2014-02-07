#include "ParentDB.h"
#include "DB.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "json.h"
#include "Config.h"
#include "Offer.h"

ParentDB::ParentDB()
{
    pdb = Config::Instance()->pDb->pDatabase;
}

ParentDB::~ParentDB()
{
    //dtor
}
void ParentDB::loadRating(const std::string &id)
{
    mongo::DB db;
    std::unique_ptr<mongo::DBClientCursor> cursor;
    Kompex::SQLiteStatement *pStmt;
    char *pData;
    int sz;
    long long id_inf;

    if(!id.size())
    {
        cursor = db.query("informer.rating", mongo::Query());
    }
    else
    {
        std::string str = "{guid_int: "+id+"}";
        cursor = db.query("informer.rating",str.c_str());
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();

    if(id.size())
    {
        try
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Informer2OfferRating WHERE id_inf='%s';",id.c_str());
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Offers::loadReting DELETE FROM Informer2OfferRating error: %s", ex.GetString().c_str());
        }
    }

    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO Informer2OfferRating(id_inf,id_ofr,rating) VALUES(");
    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        id_inf = x.getField("guid_int").numberLong();

        mongo::BSONObjIterator it = x.getObjectField("rating_int");

        while (it.more())
        {
            mongo::BSONElement e = it.next();
            std::string s = e.toString();
            s.replace(s.find(":"), 1, ",");

            bzero(pData,sz);
            sqlite3_snprintf(sz,pData,"%lli,%s);",id_inf,s.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Offers::loadReting insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Rating loaded");
}

/** Загружает все товарные предложения из MongoDb */
void ParentDB::OfferLoad(mongo::Query q_correct)
{
    mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    char *pData;
    int sz, i = 0;
    int skipped = 0;

    auto cursor = db.query("offer", q_correct);

    pStmt = new Kompex::SQLiteStatement(pdb);


    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO Offer(id,guid,campaignId,categoryId,accountId,rating,retargeting,image,height,width,isOnClick,cost\
             ,uniqueHits,swf,description,price,url,title,type,valid) VALUES(");
    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
        if (id.empty())
        {
            skipped++;
            continue;
        }

        std::string image = x.getStringField("image");
        std::string swf = x.getStringField("swf");
        if (image.empty())
        {
            if (swf.empty())
            {
                skipped++;
                continue;
            }
        }

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,
                         "%lli,'%q',%lli,%lli,'%q',%f,%d,'%q',%d,%d,%d,%f,%d,'%q','%q','%q','%q','%q',%d,%d);",
                         x.getField("guid_int").numberLong(),
                         id.c_str(),
                         x.getField("campaignId_int").numberLong(),
                         x.getField("category").numberLong(),
                         x.getStringField("accountId"),
                         mongo::DB::toFloat(x.getField("full_rating")),
                         x.getBoolField("retargeting") ? 1 : 0,
                         x.getStringField("image"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getBoolField("isOnClick") ? 1 : 0,
                         mongo::DB::toFloat(x.getField("cost")),
                         x.getIntField("uniqueHits"),
                         swf.c_str(),
                         x.getStringField("description"),
                         x.getStringField("price"),
                         x.getStringField("url"),
                         x.getStringField("title"),
                         Offer::typeFromString(x.getStringField("type")),
                         1
                        );

        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Offers::_loadFromQuery insert(%s) error: %s", buf, ex.GetString().c_str());
            skipped++;
        }

        i++;

    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Loaded %d offers", i);
    if (skipped)
        Log::warn("Offers with empty id or image skipped: %d", skipped);
}

void ParentDB::OfferRemove(const std::string &id)
{
    Kompex::SQLiteStatement *pStmt;

    if(id.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Offer WHERE guid='%s';",id.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("Offer::remove(%s) error: %s", buf, ex.GetString().c_str());
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("offer %s removed",id.c_str());
}
//-------------------------------------------------------------------------------------------------------
long long ParentDB::insertAndGetDomainId(const std::string &domain)
{
    Kompex::SQLiteStatement *pStmt;
    long long domainId = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);

    bzero(buf,sizeof(buf));
    sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q');",domain.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("ParentDB::insertAndGetDomainId insert(%s) error: %s", buf, ex.GetString().c_str());
    }

    try
    {
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,"SELECT id FROM Domains WHERE name='%q';", domain.c_str());

        pStmt->Sql(buf);
        pStmt->FetchRow();
        domainId = pStmt->GetColumnInt64(0);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("ParentDB::insertAndGetDomainId insert(%s) error: %s", buf, ex.GetString().c_str());
    }
    pStmt->FreeQuery();
    return domainId;
}
//-------------------------------------------------------------------------------------------------------
long long ParentDB::insertAndGetAccountId(const std::string &accout)
{
    Kompex::SQLiteStatement *pStmt;
    long long accountId = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);

    bzero(buf,sizeof(buf));
    sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Accounts(name) VALUES('%q');",accout.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("ParentDB::insertAndGetAccountId insert(%s) error: %s", buf, ex.GetString().c_str());
    }

    try
    {
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,"SELECT id FROM Accounts WHERE name='%q';", accout.c_str());

        pStmt->Sql(buf);
        pStmt->FetchRow();
        accountId = pStmt->GetColumnInt64(0);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("ParentDB::insertAndGetAccountId insert(%s) error: %s", buf, ex.GetString().c_str());
    }
    pStmt->FreeQuery();
    return accountId;
}
//-------------------------------------------------------------------------------------------------------
/** Загружает данные обо всех информерах */
bool ParentDB::InformerLoadAll()
{
    mongo::DB db;
    std::unique_ptr<mongo::DBClientCursor> cursor = db.query("informer", mongo::Query());
    int skipped = 0;
    Kompex::SQLiteStatement *pStmt;
    int i = 0;
    long long domainId,accountId;

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

        domainId = 0;
        accountId = 0;
        domainId = insertAndGetDomainId(x.getStringField("domain"));
        accountId = insertAndGetAccountId(x.getStringField("user"));

        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Informer(id,guid,title,bannersCss,teasersCss,domainId,accountId,blocked,\
             nonrelevant,valid,height,width,height_banner,width_banner,capacity,rtgPercentage) VALUES(\
             %lld,'%q','%q','%q','%q',%lld,%lld,0,'%q',1,%d,%d,%d,%d,%d,%d);",
                         x.getField("guid_int").numberLong(),
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("css_banner"),
                         x.getStringField("css"),
                         domainId,
                         accountId,
                         x.getStringField("nonRelevant"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getIntField("height_banner"),
                         x.getIntField("width_banner"),
                         capacity,
                         x.getIntField("rtgPercentage") > 0 ? x.getIntField("rtgPercentage") : 0
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

bool ParentDB::InformerUpdate(const std::string &id)
{
    mongo::DB db;
    std::unique_ptr<mongo::DBClientCursor> cursor = db.query("informer", QUERY("guid" << id));
    Kompex::SQLiteStatement *pStmt;
    long long domainId,accountId, long_id = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
        boost::to_lower(id);
        if (id.empty())
        {
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

        domainId = 0;
        accountId = 0;
        domainId = insertAndGetDomainId(x.getStringField("domain"));
        accountId = insertAndGetAccountId(x.getStringField("user"));

               sqlite3_snprintf(sizeof(buf),buf,
                         "UPDATE Informer SET\
                         title='%q',\
                         bannersCss='%q',\
                         teasersCss='%q',\
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
                         rtgPercentage=%d \
                         WHERE id=%lld;",
                         x.getStringField("title"),
                         x.getStringField("css_banner"),
                         x.getStringField("css"),
                         domainId,
                         accountId,
                         x.getStringField("nonRelevant"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getIntField("height_banner"),
                         x.getIntField("width_banner"),
                         capacity,
                         x.getIntField("rtgPercentage"),
                         long_id
                         );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::_loadFromQuery insert error: %s", ex.GetString().c_str());
        }
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("updated informer id %lld", long_id);
    return true;
}


void ParentDB::InformerRemove(const std::string &id)
{
    Kompex::SQLiteStatement *pStmt;

    if(id.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Informer WHERE guid='%s';",id.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::remove(%s) error: %s", buf, ex.GetString().c_str());
        }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("informer %s removed",id.c_str());
}
/*
void ParentDB::updateRating(const std::string &id)
{
    Kompex::SQLiteStatement *pStmt;

    if(id.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Informer2OfferRating WHERE id_inf=%s;",id.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Informer::remove(%s) error: %s", buf, ex.GetString().c_str());
        }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("informer %s removed",id.c_str());
}
*/
void ParentDB::CategoriesLoad()
{
    mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    int i = 0;

    auto cursor = db.query("domain.categories", mongo::Query());

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string catAll;
        mongo::BSONObjIterator it = x.getObjectField("categories");
        while (it.more())
        {
            std::string cat = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Categories(id,guid) VALUES(%lli,'%q');",
                             i, cat.c_str());
            try
            {
                pStmt->SqlStatement(buf);
                i++;
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("CategoriesLoad insert(%s) error: %s", buf, ex.GetString().c_str());
            }
            catAll += "'"+cat+"',";
        }

//domain
        long long domainId = insertAndGetDomainId(x.getStringField("domain"));

        catAll = catAll.substr(0, catAll.size()-1);
        sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Categories2Domain(id_dom,id_cat) \
                         SELECT %lld,id FROM Categories WHERE guid IN(%s);",
                         domainId,catAll.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("CategoriesLoad insert(%s) error: %s", buf, ex.GetString().c_str());
        }

    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Loaded %d categories", i);
}
