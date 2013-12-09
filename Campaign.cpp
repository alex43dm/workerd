#include <boost/algorithm/string/replace.hpp>

#include "Campaign.h"
#include "CampaignShowOptions.h"
#include "DB.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

/** \brief  Конструктор

    \param id        Идентификатор рекламной кампании
*/
Campaign::Campaign(const std::string &id)
{
    d = cache()[id];
    if (!d)
    {
        d = new CampaignData(id);
        cache()[id] = d;
    }
}

/** \brief  Закгрузка всех рекламных кампаний из базы данных  Mongo

 */
#define CAMINS
void Campaign::loadAll(Kompex::SQLiteDatabase *pdb)
{
    CampaignOptionsCache::invalidate();
    mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *pData;
    int sz, i = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);



    auto cursor = db.query("campaign", mongo::Query());

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
        snprintf(buf,sizeof(buf),"INSERT INTO Campaign(id,guid,title,project,social,valid,showCoverage,impressionsPerDayLimit) VALUES(");

        sz = strlen(buf);
        pData = buf + sz;
        sz = sizeof(buf) - sz;

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,
                         "%lld,'%q','%q','%q',%d, %d,'%q',%d)",
                         long_id,
                         id.c_str(),
                         x.getStringField("title"),
                         x.getStringField("project"),
                         x.getBoolField("social") ? 1 : 0,
                         o.isValid(),
                         o.getStringField("showCoverage"),
                         x.getField("impressionsPerDayLimit").numberInt()
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

        bzero(buf,sizeof(buf));

        mongo::BSONObjIterator it = o.getObjectField("geoTargeting");
        std::string country_targeting;
        while (it.more())
            country_targeting += "'" + it.next().str() + "',";

        country_targeting = country_targeting.substr(0, country_targeting.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO geoTargeting(id_cam,id_geo) \
                    SELECT %lld,id FROM GeoRerions WHERE cid IN(%s)",
                         long_id, country_targeting.c_str()
                        );
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
        }

        //------------------------regionTargeting-----------------------

        bzero(buf,sizeof(buf));

        it = o.getObjectField("regionTargeting");
        std::string region_targeting;
        while (it.more())
        {
            std::string rep = it.next().str();
            boost::replace_all(rep,"'", "''");
            region_targeting += "'" + rep + "',";
        }

        region_targeting = region_targeting.substr(0, region_targeting.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO regionTargeting(id_cam,id_geo) \
                    SELECT %lld,id FROM GeoRerions WHERE rname IN(%s)",
                         long_id, region_targeting.c_str()
                        );
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
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Accounts(name) VALUES('%q')",it.next().str().c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            accounts_allowed += "'"+it.next().str()+"',";
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
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Accounts(name) VALUES('%q')",it.next().str().c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            accounts_ignored += "'"+it.next().str()+"',";
        }

        accounts_ignored = accounts_ignored.substr(0, accounts_ignored.size()-1);
        bzero(buf,sizeof(buf));
        sqlite3_snprintf(sizeof(buf),buf,
                         "INSERT INTO Campaign2Accounts(id_cam,id_acc,allowed) \
                         SELECT %lld,id,0 FROM Accounts WHERE name IN(%s)",
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
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Domains(name) VALUES('%q')",it.next().str().c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            domains_allowed += "'"+it.next().str()+"',";
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
            sqlite3_snprintf(sizeof(buf),buf,"INSERT INTO Domains(name) VALUES('%q')",it.next().str().c_str());
            try
            {
                pStmt->SqlStatement(buf);
            }
            catch(Kompex::SQLiteException &ex)
            {
                Log::err("Campaign::loadAll insert(%s) error: %s", buf, ex.GetString().c_str());
            }

            domains_ignored += "'"+it.next().str()+"',";
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
            bzero(buf,sizeof(buf));
            sqlite3_snprintf(sizeof(buf),buf,
                             "INSERT INTO CronCampaign(id_cam,Day,Hour,Min,startStop) VALUES(%lld,%d,%d,%d,1)",
                             long_id, it.next().numberInt(),
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
                             long_id, it.next().numberInt(),
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
/*
 // Тематические категории, к которым относится кампания
    it = o.getObjectField("categories");
    while (it.more())
        categories_.insert(it.next().str());
*/
        i++;
    }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("Loaded %d campaigns",i);
}
