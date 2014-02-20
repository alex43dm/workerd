#include <boost/algorithm/string/replace.hpp>

#include "Campaign.h"
#include "DB.h"
#include "Log.h"
#include "Config.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

/** \brief  Конструктор

    \param id        Идентификатор рекламной кампании
*/
Campaign::Campaign(long long _id) :
    id(_id)
{
    mongo::DB db;

    auto cursor = db.query("campaign", QUERY("guid_int" << _id));

    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();

        guid = x.getStringField("guid");
        if (guid.empty())
        {
            Log::warn("Campaign with empty guid");
        }

        mongo::BSONObj o = x.getObjectField("showConditions");

        id = x.getField("guid_int").numberLong();
        title = x.getStringField("title");
        project = x.getStringField("project");
        social = x.getBoolField("social") ? 1 : 0;
        valid = o.isValid();
        //o.getStringField("showCoverage");
        //x.getField("impressionsPerDayLimit").numberInt()
    }
}
//-------------------------------------------------------------------------------------------------------
void Campaign::info(std::vector<Campaign*> &res)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    sqlite3_snprintf(sizeof(buf),buf,
            "SELECT c.title,c.valid,c.social,count(ofr.campaignId) FROM Campaign AS c \
            LEFT JOIN Offer AS ofr ON c.id=ofr.campaignId \
            GROUP BY ofr.campaignId;");

    try
    {
        pStmt->Sql(buf);

        while(pStmt->FetchRow())
        {
            Campaign *c = new Campaign();
            c->title = pStmt->GetColumnString(0);
            c->valid = pStmt->GetColumnInt(1) ? true : false;
            c->social = pStmt->GetColumnInt(2) ? true : false;
            c->offersCount = pStmt->GetColumnInt(3);
            res.push_back(c);
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("Campaign::info %s error: %s", buf, ex.GetString().c_str());
    }

    pStmt->FreeQuery();
}

//-------------------------------------------------------------------------------------------------------
void Campaign::GeoRerionsAdd(const std::string &country, const std::string &region)
{
    char buf[8192];
    Kompex::SQLiteStatement *pStmt;

    pStmt = new Kompex::SQLiteStatement(Config::Instance()->pDb->pDatabase);

    sqlite3_snprintf(sizeof(buf),buf,
            "INSERT INTO GeoLiteCity(locId,country,city) SELECT max(locId)+1,'%q','%q' FROM GeoLiteCity;",
            country.c_str(),
            region.c_str());
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("GeoRerions::add %s error: %s", buf, ex.GetString().c_str());
    }
}

/** \brief  Закгрузка всех рекламных кампаний из базы данных  Mongo

 */
//==================================================================================
void Campaign::loadAll(Kompex::SQLiteDatabase *pdb, mongo::Query q_correct)
{
    mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *pData;
    int sz, i = 0, cats = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);

    auto cursor = db.query("campaign", q_correct);

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
        snprintf(buf,sizeof(buf),"INSERT INTO Campaign(id,guid,title,project,social,valid,showCoverage,impressionsPerDayLimit,retargeting) VALUES(");

        sz = strlen(buf);
        pData = buf + sz;
        sz = sizeof(buf) - sz;

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,
                         "%lld,'%q','%q','%q',%d, %d,'%q',%d,%d)",
                         long_id,
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("project"),
                         x.getBoolField("social") ? 1 : 0,
                         o.isValid(),
                         o.getStringField("showCoverage"),
                         x.getField("impressionsPerDayLimit").numberInt(),
                         o.getBoolField("retargeting") ? 1 : 0
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }
        //------------------------geoTargeting-----------------------
        mongo::BSONObjIterator it = o.getObjectField("geoTargeting");
        std::string country_targeting;
        while (it.more())
            country_targeting += "'" + it.next().str() + "',";

        country_targeting = country_targeting.substr(0, country_targeting.size()-1);

        //------------------------regionTargeting-----------------------
        it = o.getObjectField("regionTargeting");
        std::string region_targeting;
        while (it.more())
        {
            std::string rep = it.next().str();
            GeoRerionsAdd("", rep);
            boost::replace_all(rep,"'", "''");
            region_targeting += "'" + rep + "',";
        }

        region_targeting = region_targeting.substr(0, region_targeting.size()-1);
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
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO geoTargeting(id_cam,id_geo) \
                              SELECT %lld,locId FROM GeoLiteCity WHERE country IN(%s) AND city='';",
                             long_id, country_targeting.c_str()
                            );
        }

        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }

        //------------------------informers-----------------------

        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string informers_allowed;
        it = o.getObjectField("allowed").getObjectField("informers");
        while (it.more())
            informers_allowed += "'"+it.next().str()+"',";

        informers_allowed = informers_allowed.substr(0, informers_allowed.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Campaign2Informer(id_cam,id_inf,allowed) \
                         SELECT %lld,id,1 FROM Informer WHERE guid IN(%s)",
                         long_id, informers_allowed.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
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
                         SELECT %lld,id,0 FROM Informer WHERE guid IN(%s)",
                         long_id, informers_ignored.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }

        //------------------------accounts-----------------------

        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string accounts_allowed;
        it = o.getObjectField("allowed").getObjectField("accounts");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Accounts(name) VALUES('%q')",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            accounts_allowed += "'"+acnt+"',";
        }


        accounts_allowed = accounts_allowed.substr(0, accounts_allowed.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) \
                         SELECT %lld,id,1 FROM Accounts WHERE name IN(%s)",
                         long_id, accounts_allowed.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
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
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            accounts_ignored += "'"+acnt+"',";
        }

        accounts_ignored = accounts_ignored.substr(0, accounts_ignored.size()-1);
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
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }


        //------------------------domains-----------------------

        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string domains_allowed;
        it = o.getObjectField("allowed").getObjectField("domains");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q')",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            domains_allowed += "'"+acnt+"',";
        }

        domains_allowed = domains_allowed.substr(0, domains_allowed.size()-1);
        if(domains_allowed.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Domains(id_cam,id_dom,allowed) \
                             SELECT %lld,id,1 FROM Domains WHERE name IN(%s)",
                             long_id, domains_allowed.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

        // Множества информеров, аккаунтов и доменов, на которых запрещен показ
        std::string domains_ignored;
        it = o.getObjectField("ignored").getObjectField("domains");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q')",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            domains_ignored += "'"+acnt+"',";
        }

        domains_ignored = domains_ignored.substr(0, domains_ignored.size()-1);
        if(domains_ignored.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Domains(id_cam,id_dom,allowed) \
                             SELECT %lld,id,0 FROM Domains WHERE name IN(%s)",
                             long_id, domains_ignored.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

        // Дни недели, в которые осуществляется показ
        it = o.getObjectField("daysOfWeek");
        int day;
        if (!it.more())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,null,%d,%d,1)",
                             long_id,
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.minutes"))
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,null,%d,%d,0)",
                             long_id,
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.minutes"))
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

        while (it.more())
        {
            day = it.next().numberInt();
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,1)",
                             long_id, day,
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.minutes"))
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,0)",
                             long_id, day,
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.minutes"))
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

 // Тематические категории, к которым относится кампания
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
                Log::err("CategoriesLoad insert(%s) error: %s", buf, ex.GetString().c_str());
            }
            catAll += "'"+cat+"',";
        }

        catAll = catAll.substr(0, catAll.size()-1);
        sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Campaign2Categories(id_cam,id_cat) \
                         SELECT %lld,cat.id FROM Categories AS cat WHERE cat.guid IN(%s);",
                         long_id,catAll.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("CategoriesLoad insert(%s) error: %s", buf, ex.GetString().c_str());
        }

        i++;
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("Loaded %d campaigns",i);
}

//==================================================================================
/** \brief  Обновление кампании из базы данных  Mongo

 */
void Campaign::update(Kompex::SQLiteDatabase *pdb, std::string aCampaignId)
{

    mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];
    int cats = 0;
    long long long_id;

    pStmt = new Kompex::SQLiteStatement(pdb);

    auto cursor = db.query("campaign", QUERY("guid" << aCampaignId));

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

        long_id = x.getField("guid_int").numberLong();

        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,"UPDATE Campaign SET \
                             title='%q', \
                             project='%q', \
                             social=%d, \
                             valid=%d, \
                             showCoverage='%q', \
                             impressionsPerDayLimit=%d, \
                             retargeting=%d \
                             WHERE id=%lld;",
                             x.getStringField("title"),
                             x.getStringField("project"),
                             x.getBoolField("social") ? 1 : 0,
                             o.isValid(),
                             o.getStringField("showCoverage"),
                             x.getField("impressionsPerDayLimit").numberInt(),
                             long_id,
                             o.getBoolField("retargeting") ? 1 : 0);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }
        //------------------------geoTargeting-----------------------
        sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM geoTargeting WHERE id_cam=%lld;",long_id);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll update(%s) error: %s", buf, ex.GetString().c_str());
        }

        mongo::BSONObjIterator it = o.getObjectField("geoTargeting");
        std::string country_targeting;
        while (it.more())
            country_targeting += "'" + it.next().str() + "',";

        country_targeting = country_targeting.substr(0, country_targeting.size()-1);

        //------------------------regionTargeting-----------------------
        it = o.getObjectField("regionTargeting");
        std::string region_targeting;
        while (it.more())
        {
            std::string rep = it.next().str();
            GeoRerionsAdd("", rep);
            boost::replace_all(rep,"'", "''");
            region_targeting += "'" + rep + "',";
        }

        region_targeting = region_targeting.substr(0, region_targeting.size()-1);
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
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO geoTargeting(id_cam,id_geo) \
                              SELECT %lld,locId FROM GeoLiteCity WHERE country IN(%s) AND city='';",
                             long_id, country_targeting.c_str()
                            );
        }

        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }

        //------------------------informers-----------------------
        sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Informer WHERE id_cam=%lld;",long_id);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll update(%s) error: %s", buf, ex.GetString().c_str());
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
                         SELECT %lld,id,1 FROM Informer WHERE guid IN(%s)",
                         long_id, informers_allowed.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
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
                         SELECT %lld,id,0 FROM Informer WHERE guid IN(%s)",
                         long_id, informers_ignored.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }

        //------------------------accounts-----------------------
        sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Accounts WHERE id_cam=%lld;",long_id);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll update(%s) error: %s", buf, ex.GetString().c_str());
        }
        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string accounts_allowed;
        it = o.getObjectField("allowed").getObjectField("accounts");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Accounts(name) VALUES('%q')",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            accounts_allowed += "'"+acnt+"',";
        }


        accounts_allowed = accounts_allowed.substr(0, accounts_allowed.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) \
                         SELECT %lld,id,1 FROM Accounts WHERE name IN(%s)",
                         long_id, accounts_allowed.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
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
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            accounts_ignored += "'"+acnt+"',";
        }

        accounts_ignored = accounts_ignored.substr(0, accounts_ignored.size()-1);
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
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }


        //------------------------domains-----------------------

        // Множества информеров, аккаунтов и доменов, на которых разрешён показ
        std::string domains_allowed;
        it = o.getObjectField("allowed").getObjectField("domains");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q')",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            domains_allowed += "'"+acnt+"',";
        }

        domains_allowed = domains_allowed.substr(0, domains_allowed.size()-1);
        if(domains_allowed.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Domains(id_cam,id_dom,allowed) \
                             SELECT %lld,id,1 FROM Domains WHERE name IN(%s)",
                             long_id, domains_allowed.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

        // Множества информеров, аккаунтов и доменов, на которых запрещен показ
        std::string domains_ignored;
        it = o.getObjectField("ignored").getObjectField("domains");
        while (it.more())
        {
            bzero(buf,sizeof(buf));
            std::string acnt = it.next().str();
            sqlite3_snprintf(sizeof(buf),buf,"INSERT OR IGNORE INTO Domains(name) VALUES('%q')",acnt.c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            domains_ignored += "'"+acnt+"',";
        }

        domains_ignored = domains_ignored.substr(0, domains_ignored.size()-1);
        if(domains_ignored.size())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO Campaign2Domains(id_cam,id_dom,allowed) \
                             SELECT %lld,id,0 FROM Domains WHERE name IN(%s)",
                             long_id, domains_ignored.c_str()
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

        // Дни недели, в которые осуществляется показ
        sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM CronCampaign WHERE id_cam=%lld;",long_id);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll update(%s) error: %s", buf, ex.GetString().c_str());
        }

        it = o.getObjectField("daysOfWeek");
        int day;
        if (!it.more())
        {
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,null,%d,%d,1)",
                             long_id,
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.minutes"))
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,null,%d,%d,0)",
                             long_id,
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.minutes"))
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

        while (it.more())
        {
            day = it.next().numberInt();
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,1)",
                             long_id, day,
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("startShowTime.minutes"))
                            );

            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,0)",
                             long_id, day,
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.hours")),
                             mongo::DB::toInt(o.getFieldDotted("endShowTime.minutes"))
                            );
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }
        }

 // Тематические категории, к которым относится кампания
        sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign2Categories WHERE id_cam=%lld;",long_id);
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll update(%s) error: %s", buf, ex.GetString().c_str());
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
                Log::err("CategoriesLoad insert(%s) error: %s", buf, ex.GetString().c_str());
            }
            catAll += "'"+cat+"',";
        }

        catAll = catAll.substr(0, catAll.size()-1);
        sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Campaign2Categories(id_cam,id_cat) \
                         SELECT %lld,cat.id FROM Categories AS cat WHERE cat.guid IN(%s);",
                         long_id,catAll.c_str());
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

    Log::info("campaign %lld updated",long_id);
}

/** \brief  Обновление кампании из базы данных  Mongo

 */
void Campaign::startStop(Kompex::SQLiteDatabase *pdb,
        std::string aCampaignId,
        int StartStop)
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];

    if(aCampaignId.empty())
    {
        return;
    }

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
        Log::err("Campaign::start update(%s) error: %s", buf, ex.GetString().c_str());
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("campaign %s %sed",aCampaignId.c_str(), StartStop ? "start":"stop");
}

//==================================================================================
std::string Campaign::getName(long long campaign_id)
{
    mongo::DB db;

    auto cursor = db.query("campaign", QUERY("guid_int" << campaign_id));

    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        return x.getStringField("guid");
    }
    return "";
}

void Campaign::remove(Kompex::SQLiteDatabase *pdb, std::string aCampaignId)
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];

    if(aCampaignId.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Campaign WHERE guid='%s';",aCampaignId.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::remove(%s) error: %s", buf, ex.GetString().c_str());
        }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("campaign %s removed",aCampaignId.c_str());
}

