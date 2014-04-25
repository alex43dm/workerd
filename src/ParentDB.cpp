#include <vector>
#include <boost/algorithm/string.hpp>

#include "ParentDB.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "json.h"
#include "Config.h"
#include "Offer.h"

ParentDB::ParentDB()
{
    pdb = Config::Instance()->pDb->pDatabase;
    fConnectedToMainDatabase = false;
    ConnectMainDatabase();
}

ParentDB::~ParentDB()
{
    //dtor
}


bool ParentDB::ConnectMainDatabase()
{
    if(fConnectedToMainDatabase)
        return true;

    std::vector<mongo::HostAndPort> hvec;
    for(auto h = cfg->mongo_main_host_.begin(); h != cfg->mongo_main_host_.end(); ++h)
    {
        hvec.push_back(mongo::HostAndPort(*h));
        Log::info("Connecting to: %s",(*h).c_str());
    }

    try
    {
        std::string err;
        if(!cfg->mongo_main_set_.empty())
        {
            monga_main = new mongo::DBClientReplicaSet(cfg->mongo_main_set_, hvec);
            monga_main->connect();
        }
        else
        {
//            monga_main = new mongo::DBClientConnection();
//            monga_main->connect((*cfg->mongo_main_host_.begin()), err);
        }


        if(!cfg->mongo_main_login_.empty())
        {
            if(!monga_main->auth(cfg->mongo_main_db_,cfg->mongo_main_login_,cfg->mongo_main_passwd_, err))
            {
                Log::warn("auth db: %s login: %s error: %s",
                          cfg->mongo_main_db_.c_str(),cfg->mongo_main_login_.c_str(), err.c_str());
            }
            else
            {
                fConnectedToMainDatabase = true;
            }
        }
        else
        {
            fConnectedToMainDatabase = true;
        }
    }
    catch (mongo::UserException &ex)
    {
        Log::err("ParentDB mongo error: %s", ex.what());
        return false;
    }

    return true;
}

void ParentDB::loadRating(const std::string &id)
{
    if(!fConnectedToMainDatabase)
        return;

    std::unique_ptr<mongo::DBClientCursor> cursor;
    Kompex::SQLiteStatement *pStmt;
    char *pData;
    int sz;
    long long id_inf;

    if(!id.size())
    {
        cursor = monga_main->query(cfg->mongo_main_db_ + ".informer.rating", mongo::Query());
    }
    else
    {
        std::string str = "{guid_int: "+id+"}";
        cursor = monga_main->query(cfg->mongo_main_db_ + ".informer.rating",str.c_str());
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
            logDb(ex);
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
                logDb(ex);
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
    if(!fConnectedToMainDatabase)
        return;

    //mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    char *pData;
    int sz, i = 0;
    int skipped = 0;

    auto cursor = monga_main->query(cfg->mongo_main_db_ + ".offer", q_correct);

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
                         x.getField("full_rating").numberDouble(),
                         x.getBoolField("retargeting") ? 1 : 0,
                         x.getStringField("image"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getBoolField("isOnClick") ? 1 : 0,
                         x.getField("cost").numberDouble(),
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
            logDb(ex);
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
        logDb(ex);
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
        logDb(ex);
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
        logDb(ex);
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
        logDb(ex);
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
        logDb(ex);
    }
    pStmt->FreeQuery();
    return accountId;
}
//-------------------------------------------------------------------------------------------------------
/** Загружает данные обо всех информерах */
bool ParentDB::InformerLoadAll()
{
    if(!fConnectedToMainDatabase)
        return false;

    std::unique_ptr<mongo::DBClientCursor> cursor = monga_main->query(cfg->mongo_main_db_ + ".informer", mongo::Query());
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
                         0
//                         x.getIntField("rtgPercentage") > 0 ? x.getIntField("rtgPercentage") : 0
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
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
    if(!fConnectedToMainDatabase)
        return false;

    std::unique_ptr<mongo::DBClientCursor> cursor = monga_main->query(cfg->mongo_main_db_ + ".informer", QUERY("guid" << id));
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
                         0,//x.getIntField("rtgPercentage"),
                         long_id
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
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
        logDb(ex);
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
            logDb(ex);
        }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("informer %s removed",id.c_str());
}
*/
void ParentDB::CategoriesLoad()
{
    if(!fConnectedToMainDatabase)
        return;

    Kompex::SQLiteStatement *pStmt;
    int i = 0;

    auto cursor = monga_main->query(cfg->mongo_main_db_ + ".domain.categories", mongo::Query());

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
                logDb(ex);
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
            logDb(ex);
        }

    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Loaded %d categories", i);
}


//-------------------------------------------------------------------------------------------------------
void ParentDB::GeoRerionsAdd(const std::string &country, const std::string &region)
{
    Kompex::SQLiteStatement *pStmt;
    std::vector<std::string> val;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    boost::split(val, country, boost::is_any_of(","));

    for(unsigned i=0; i < val.size(); i++)
    {
        sqlite3_snprintf(sizeof(buf),buf,
                         "SELECT locId FROM GeoLiteCity WHERE country=%s AND city='%q';",
                         val[i].size() > 0 ? val[i].c_str() : "''",
                         region.c_str());
        try
        {
            pStmt->SqlStatement(buf);

            if(pStmt->GetDataCount() == 0)
            {
                sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO GeoLiteCity(locId,country,city) \
                         SELECT max(locId)+1,%s,'%q' FROM GeoLiteCity;",
                             val[i].size() > 0 ? val[i].c_str() : "''",
                             region.c_str());
                pStmt->SqlStatement(buf);
            }

        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }
    }
}


void ParentDB::logDb(const Kompex::SQLiteException &ex) const
{
    Log::err("ParentDB::logDb request:(%s) error: %s", buf, ex.GetString().c_str());
}
/** \brief  Закгрузка всех рекламных кампаний из базы данных  Mongo

 */
//==================================================================================
void ParentDB::CampaignLoad(const std::string &aCampaignId)
{
    std::unique_ptr<mongo::DBClientCursor> cursor;
    Kompex::SQLiteStatement *pStmt;
    int i = 0, cats = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);

    if(!aCampaignId.empty())
    {
        cursor = monga_main->query(cfg->mongo_main_db_ +".campaign", QUERY("guid" << aCampaignId));
    }
    else
    {
        cursor = monga_main->query(cfg->mongo_main_db_ +".campaign", mongo::Query());
    }

    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
        if (id.empty())
        {
            Log::warn("Campaign with empty id skipped");
            continue;
        }

        mongo::BSONObj o = x.getObjectField("showConditions");

        long long long_id = x.getField("guid_int").numberLong();

        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT OR REPLACE INTO Campaign\
                         (id,guid,title,project,social,valid,showCoverage,impressionsPerDayLimit,retargeting) \
                         VALUES(%lld,'%q','%q','%q',%d,%d,%d,%d,%d);",
                         long_id,
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("project"),
                         x.getBoolField("social") ? 1 : 0,
                         o.isValid() ? 1 : 0,
                         Campaign::typeConv(o.getStringField("showCoverage")),
                         x.getField("impressionsPerDayLimit").numberInt(),
                         o.getBoolField("retargeting") ? 1 : 0
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }

        //------------------------geoTargeting-----------------------
        if(!aCampaignId.empty())
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM geoTargeting WHERE id_cam=%lld;",long_id);
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        mongo::BSONObjIterator it = o.getObjectField("geoTargeting");
        std::string country_targeting;
        while (it.more())
        {
            if(country_targeting.empty())
            {
                country_targeting += "'"+it.next().str()+"'";
            }
            else
            {
                country_targeting += ",'"+it.next().str()+"'";
            }
        }

        //------------------------regionTargeting-----------------------
        it = o.getObjectField("regionTargeting");
        std::string region_targeting;
        while (it.more())
        {
            std::string rep = it.next().str();
            GeoRerionsAdd(country_targeting, rep);
            boost::replace_all(rep,"'", "''");

            if(region_targeting.empty())
            {
                region_targeting += "'"+rep+"'";
            }
            else
            {
                region_targeting += ",'"+rep+"'";
            }
        }

        if(region_targeting.size())
        {
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO geoTargeting(id_cam,id_geo) \
                              SELECT %lld,locId FROM GeoLiteCity WHERE city IN(%s);",
                             long_id,region_targeting.c_str()
                            );
        }
        else
        {
            if(country_targeting.size())
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO geoTargeting(id_cam,id_geo) \
                                  SELECT %lld,locId FROM GeoLiteCity WHERE country IN(%s) AND city='';",
                                 long_id, country_targeting.c_str()
                                );
            }
            else
            {
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO geoTargeting(id_cam,id_geo) VALUES(%lld,1);",
                                 long_id
                                );
            }
        }

        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }

        //------------------------informers-----------------------
        if(!aCampaignId.empty())
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Informer WHERE id_cam=%lld;",long_id);
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }
        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string informers_allowed;
        it = o.getObjectField("allowed").getObjectField("informers");
        while (it.more())
            informers_allowed += "'"+it.next().str()+"',";

        informers_allowed = informers_allowed.substr(0, informers_allowed.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Campaign2Informer(id_cam,id_inf,allowed) \
                         SELECT %lld,id,1 FROM Informer WHERE guid IN(%s);",
                         long_id, informers_allowed.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }

        bzero(buf,sizeof(buf));

        // Множества информеров, аккаунтов и доменов, на которых запрещен показ
        std::string informers_ignored;
        it = o.getObjectField("ignored").getObjectField("informers");
        while (it.more())
            informers_ignored += "'"+it.next().str()+"',";

        informers_ignored = informers_ignored.substr(0, informers_ignored.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Campaign2Informer(id_cam,id_inf,allowed) \
                         SELECT %lld,id,0 FROM Informer WHERE guid IN(%s);",
                         long_id, informers_ignored.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }

        //------------------------accounts-----------------------
        if(!aCampaignId.empty())
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Accounts WHERE id_cam=%lld;",long_id);
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }
        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string accounts_allowed;
        it = o.getObjectField("allowed").getObjectField("accounts");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Accounts(name) VALUES('%q');",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }

            if(accounts_allowed.empty())
            {
                accounts_allowed += "'"+acnt+"'";
            }
            else
            {
                accounts_allowed += ",'"+acnt+"'";
            }
        }


        if(accounts_allowed.size())
        {

            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) \
                             SELECT %lld,id,1 FROM Accounts WHERE name IN(%s);",
                             long_id, accounts_allowed.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }
        else
        {
            if(o.getObjectField("allowed").hasElement("accounts"))
            {
                bzero(buf,sizeof(buf));
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) VALUES(%lld,1,1);",
                                 long_id
                                );
                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                }
            }
        }

        // Множества информеров, аккаунтов и доменов, на которых запрещен показ
        std::string accounts_ignored;
        it = o.getObjectField("ignored").getObjectField("accounts");
        while (it.more())
        {
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Accounts(name) VALUES('%q');",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }

            if(accounts_ignored.empty())
            {
                accounts_ignored += "'"+acnt+"'";
            }
            else
            {
                accounts_ignored += ",'"+acnt+"'";
            }
        }

        if(accounts_ignored.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) \
                             SELECT %lld,id,0 FROM Accounts WHERE name IN(%s);",
                             long_id, accounts_ignored.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        else
        {
            if(o.getObjectField("ignored").hasElement("accounts"))
            {
                bzero(buf,sizeof(buf));
                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) VALUES(%lld,1,0);",
                                 long_id
                                );
                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                }
            }
        }

        //------------------------domains-----------------------
        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        if(!aCampaignId.empty())
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Domains WHERE id_cam=%lld;",long_id);
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        std::string domains_allowed;
        it = o.getObjectField("allowed").getObjectField("domains");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q');",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }

            if(domains_allowed.empty())
            {
                domains_allowed += "'"+acnt+"'";
            }
            else
            {
                domains_allowed += ",'"+acnt+"'";
            }
        }

        if(domains_allowed.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Domains(id_cam,id_dom,allowed) \
                             SELECT %lld,id,1 FROM Domains WHERE name IN(%s);",
                             long_id, domains_allowed.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        // Множества информеров, аккаунтов и доменов, на которых запрещен показ
        std::string domains_ignored;
        it = o.getObjectField("ignored").getObjectField("domains");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q');",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }

            if(domains_ignored.empty())
            {
                domains_ignored += "'"+acnt+"'";
            }
            else
            {
                domains_ignored += ",'"+acnt+"'";
            }
        }

        if(domains_ignored.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Domains(id_cam,id_dom,allowed) \
                             SELECT %lld,id,0 FROM Domains WHERE name IN(%s);",
                             long_id, domains_ignored.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        //------------------------cron-----------------------
        // Дни недели, в которые осуществляется показ
        if(!aCampaignId.empty())
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM CronCampaign WHERE id_cam=%lld;",long_id);
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        int day,startShowTimeHours,startShowTimeMinutes,endShowTimeHours,endShowTimeMinutes;

        mongo::BSONObj bstartTime = o.getObjectField("startShowTime");
        mongo::BSONObj bendTime = o.getObjectField("endShowTime");

        startShowTimeHours = strtol(bstartTime.getStringField("hours"),NULL,10);
        startShowTimeMinutes = strtol(bstartTime.getStringField("minutes"),NULL,10);
        endShowTimeHours = strtol(bendTime.getStringField("hours"),NULL,10);
        endShowTimeMinutes = strtol(bendTime.getStringField("minutes"),NULL,10);

        if(startShowTimeHours == 0 &&
        startShowTimeMinutes == 0 &&
        endShowTimeHours == 0 &&
        endShowTimeMinutes == 0)
        {
            endShowTimeHours = 24;
        }

        if(o.getObjectField("daysOfWeek").isEmpty())
        {
            for(day = 1; day < 8; day++)
            {
                bzero(buf,sizeof(buf));
                sqlite3_snprintf(sizeof(buf),buf,
                "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,1);",
                long_id, day,
                startShowTimeHours,
                startShowTimeMinutes
                                );

                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                }

                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,0);",
                                 long_id, day,
                                 endShowTimeHours,
                                 endShowTimeMinutes
                                );
                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                }
            }
        }
        else
        {
            it = o.getObjectField("daysOfWeek");
            while (it.more())
            {
                day = it.next().numberInt();
                bzero(buf,sizeof(buf));
                sqlite3_snprintf(sizeof(buf),buf,
                "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,1);",
                long_id, day,
                startShowTimeHours,
                startShowTimeMinutes
                                );

                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                }

                sqlite3_snprintf(sizeof(buf),buf,
                                 "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,0);",
                                 long_id, day,
                                 endShowTimeHours,
                                 endShowTimeMinutes
                                );
                try
                {
                    pStmt->SqlStatement(buf);
                }
                catch(Kompex::SQLiteException &ex)
                {
                    logDb(ex);
                }
            }
        }

        //------------------------themes-----------------------
        // Тематические категории, к которым относится кампания
        if(!aCampaignId.empty())
        {
            sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Categories WHERE id_cam=%lld;",long_id);
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
        }

        std::string catAll;
        it = o.getObjectField("categories");
        while (it.more())
        {
            std::string cat = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Categories(id,guid) VALUES(%lli,'%q');",
                             cats, cat.c_str());
            try
            {
                pStmt->SqlStatement(buf);
                cats++;
            }
            catch(Kompex::SQLiteException &ex)
            {
                logDb(ex);
            }
            if(catAll.empty())
            {
                catAll += "'"+cat+"'";
            }
            else
            {
                catAll += ",'"+cat+"'";
            }
        }

        sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Campaign2Categories(id_cam,id_cat) \
                         SELECT %lld,cat.id FROM Categories AS cat WHERE cat.guid IN(%s);",
                         long_id,catAll.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            logDb(ex);
        }

        i++;
    }

    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Categories WHERE id_cam IN (SELECT id FROM Campaign WHERE showCoverage=1);");

    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("Loaded %d campaigns",i);
}

/** \brief  Обновление кампании из базы данных  Mongo

 */
void ParentDB::CampaignStartStop(const std::string &aCampaignId, int StartStop)
{
    if(aCampaignId.empty())
    {
        return;
    }

    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();

    // Дни недели, в которые осуществляется показ

    bzero(buf,sizeof(buf));
    sqlite3_snprintf(sizeof(buf),buf,"UPDATE Campaign \
                     SET valid=%d \
                     WHERE guid='%s';",StartStop,aCampaignId.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("campaign %s %sed",aCampaignId.c_str(), StartStop ? "start":"stop");
}


void ParentDB::CampaignRemove(const std::string &aCampaignId)
{
    if(aCampaignId.empty())
    {
        return;
    }

    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign WHERE guid='%s';",aCampaignId.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        logDb(ex);
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("campaign %s removed",aCampaignId.c_str());
}

//==================================================================================
std::string ParentDB::CampaignGetName(long long campaign_id)
{
    auto cursor = monga_main->query(cfg->mongo_main_db_ +".campaign", QUERY("guid_int" << campaign_id));

    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        return x.getStringField("guid");
    }
    return "";
}

