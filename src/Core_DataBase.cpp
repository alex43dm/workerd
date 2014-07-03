#include "Core_DataBase.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "Config.h"

#define CMD_SIZE 8192

Core_DataBase::Core_DataBase():
    len(CMD_SIZE)
{
    cmd = new char[len];

    tmpTable = new TempTable(cmd, len);
}

Core_DataBase::~Core_DataBase()
{
    delete tmpTable;
    delete []cmd;
}
//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getGeo(const std::string &country, const std::string &region)
{
    if(country.size() || region.size())
    {
        if(region.size())
        {
            try
            {
                Kompex::SQLiteStatement *pStmt;
                pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
                sqlite3_snprintf(len, cmd,"SELECT geo.id_cam FROM geoTargeting AS geo INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_locId_city ON geo.id_geo = reg.locId AND reg.city='%q';",region.c_str());

                pStmt->Sql(cmd);

                if(pStmt->GetDataCount() > 0)
                {
                    geo =
                        "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId AND reg.city='"+region+"'";
                }
                else if(country.size())
                {

                    geo =
                        "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId AND((reg.country='"+country+"' OR reg.country='O1') AND (reg.city='' OR reg.city='NOT FOUND'))";
                }
                pStmt->FreeQuery();
                delete pStmt;
            }
            catch(Kompex::SQLiteException &ex)
            {
                std::clog<<"Geo::compute error: "<<ex.GetString()<<std::endl;
                return false;
            }
        }
        else if(country.size())
        {

            geo =
                "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId AND((reg.country='"+country+"' OR reg.country='O1') AND (reg.city='' OR reg.city='NOT FOUND'))";
        }
    }
    return true;
}
//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getOffers(Offer::Map &items)
{
    Kompex::SQLiteStatement *pStmt;
    bool ret = true;
    all_social = true;

#ifndef DUMMY
        sqlite3_snprintf(len, cmd, cfg->offerSqlStrAll.c_str(),
                         geo.c_str(),
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->accountId,
                         informer->accountId,
                         informer->id,
                         informer->id,
                         informer->id,
                         informer->capacity);
#else
    sqlite3_snprintf(len, cmd, cfg->offerSqlStr.c_str(),
                     geo.c_str(),
                     informer->domainId,
                     informer->domainId,
                     informer->accountId,
                     informer->id,
                     informer->capacity);
#endif

#ifdef DEBUG
    printf("%s\n",cmd);
#endif // DEBUG

    pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);
    try
    {
        teasersCount = 0;
        teasersMediumRating = 0;
        teasersMaxRating = 0;

        pStmt->Sql(cmd);
        while(pStmt->FetchRow())
        {

            if(items.count(pStmt->GetColumnInt64(0)) > 0)
            {
                continue;
            }

            Offer *off = new Offer(pStmt->GetColumnString(1),
                                   pStmt->GetColumnInt64(0),
                                   pStmt->GetColumnString(2),
                                   pStmt->GetColumnString(3),
                                   pStmt->GetColumnString(4),
                                   pStmt->GetColumnString(5),
                                   pStmt->GetColumnString(6),
                                   pStmt->GetColumnString(7),
                                   pStmt->GetColumnInt64(8),
                                   pStmt->GetColumnBool(9),
                                   pStmt->GetColumnInt(10),
                                   pStmt->GetColumnDouble(11),
                                   pStmt->GetColumnBool(12),
                                   pStmt->GetColumnInt(13),
                                   pStmt->GetColumnInt(14),
                                   pStmt->GetColumnInt(15),
                                   pStmt->GetColumnBool(16),
                                   pStmt->GetColumnString(17),
                                   pStmt->GetColumnInt(18)
                                  );

            if(!off->social)
                all_social = false;

            if(off->rating > teasersMaxRating)
            {
                teasersMaxRating = off->rating;
            }
            items.insert(Offer::Pair(off->id_int,off));
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<pthread_self()<<"] error: "<<__func__
                 <<ex.GetString()
                 <<std::endl;

        ret = false;
    }


    pStmt->FreeQuery();
    delete pStmt;

    offersTotal = items.size();

    return ret;
}
//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getCampaign()
{
    bool ret = false;
    Kompex::SQLiteStatement *pStmt;

    sqlite3_snprintf(len, cmd, cfg->campaingSqlStr.c_str(),
                         tmpTable->str(),
                         geo.c_str(),
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->domainId,
                         informer->accountId,
                         informer->accountId,
                         informer->accountId,
                         informer->id,
                         informer->id,
                         informer->id,
                         informer->blocked ? " AND ca.social=1 " : ""
                         );

    pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

#ifdef DEBUG
    printf("%s\n",cmd);
#endif // DEBUG

    try
    {
        pStmt->SqlStatement(cmd);
        ret = true;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<pthread_self()<<"] error: "<<__func__
                 <<ex.GetString()
                 <<std::endl;
        ret = false;
    }


    pStmt->FreeQuery();
    delete pStmt;

    return ret;
}
//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getInformer(const std::string informer_id)
{
    bool ret = false;
    Kompex::SQLiteStatement *pStmt;

    informer = nullptr;

    pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

    sqlite3_snprintf(CMD_SIZE, cmd, cfg->informerSqlStr.c_str(), informer_id.c_str());

    try
    {
        pStmt->Sql(cmd);

        while(pStmt->FetchRow())
        {
            informer =  new Informer(pStmt->GetColumnInt64(0),
                                     pStmt->GetColumnInt(1),
                                     pStmt->GetColumnString(2),
                                     pStmt->GetColumnString(3),
                                     pStmt->GetColumnInt64(4),
                                     pStmt->GetColumnInt64(5),
                                     pStmt->GetColumnDouble(6),
                                     pStmt->GetColumnDouble(7),
                                     pStmt->GetColumnDouble(8),
                                     pStmt->GetColumnDouble(9),
                                     pStmt->GetColumnInt(10),
                                     pStmt->GetColumnBool(11)
                                    );
            ret = true;
            break;
        }
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<__func__<<" error: " <<ex.GetString()<<std::endl;
    }

    delete pStmt;

    return ret;
}
