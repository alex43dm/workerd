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

bool Core_DataBase::clearTmp()
{
    if(informer)
        delete informer;

    return tmpTable->clear();
}

//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getGeo(const std::string &country, const std::string &region)
{
    Kompex::SQLiteStatement *pStmt;

    if(country.size() || region.size())
    {
        if(region.size())
        {
            try
            {
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
                std::clog<<"["<<pthread_self()<<"] error: "<<__func__
                         <<ex.GetString()
                         <<std::endl;
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
bool Core_DataBase::getOffers(Offer::Map &items,unsigned long long sessionId)
{
    Kompex::SQLiteStatement *pStmt;
    bool ret = true;
    all_social = true;

#ifndef DUMMY
        sqlite3_snprintf(len, cmd, cfg->offerSqlStr.c_str(),
                         tmpTable->str(),
                         sessionId,
                         informer->id);
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

    try
    {
        teasersMaxRating = 0;

        pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

        pStmt->Sql(cmd);

        while(pStmt->FetchRow())
        {

            if(items.count(pStmt->GetColumnInt64(0)) > 0)
            {
                continue;
            }

            Offer *off = new Offer(pStmt->GetColumnString(1),     //guid
                                   pStmt->GetColumnInt64(0),    //id
                                   pStmt->GetColumnString(2),   //title
                                   pStmt->GetColumnString(3),   //description
                                   pStmt->GetColumnString(4),   //url
                                   pStmt->GetColumnString(5),   //image
                                   pStmt->GetColumnString(6),   //swf
                                   pStmt->GetColumnInt64(7),    //campaignId
                                   pStmt->GetColumnInt(8),      //type
                                   pStmt->GetColumnDouble(9),   //rating
                                   pStmt->GetColumnInt(10),    //uniqueHits
                                   pStmt->GetColumnInt(11),     //height
                                   pStmt->GetColumnInt(12),     //width
                                   pStmt->GetColumnBool(13),    //isOnClick
                                   pStmt->GetColumnBool(14),    //social
                                   pStmt->GetColumnString(15),  //campaign_guid
                                   pStmt->GetColumnInt(16),      //offer_by_campaign_unique
                                   false
                                  );

            if(!off->social)
                all_social = false;

            if(off->rating > teasersMaxRating)
            {
                teasersMaxRating = off->rating;
            }
            items.insert(Offer::Pair(off->id_int,off));
        }

        pStmt->FreeQuery();

        delete pStmt;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<pthread_self()<<"]"<<__func__<<" error: "
                 <<ex.GetString()
                 <<std::endl;

        ret = false;
    }



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


#ifdef DEBUG
    printf("%s\n",cmd);
#endif // DEBUG

    try
    {
        pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

        pStmt->SqlStatement(cmd);

        //pStmt->FreeQuery();
        delete pStmt;

        ret = true;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<pthread_self()<<"] error: "<<__func__
                 <<ex.GetString()
                 <<std::endl;
        ret = false;
    }



    return ret;
}
//-------------------------------------------------------------------------------------------------------------------
bool Core_DataBase::getInformer(const std::string informer_id)
{
    bool ret = false;
    Kompex::SQLiteStatement *pStmt;

    informer = nullptr;

    sqlite3_snprintf(CMD_SIZE, cmd, cfg->informerSqlStr.c_str(), informer_id.c_str());

    try
    {
        pStmt = new Kompex::SQLiteStatement(cfg->pDb->pDatabase);

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

        pStmt->FreeQuery();

        delete pStmt;
    }
    catch(Kompex::SQLiteException &ex)
    {
        std::clog<<"["<<pthread_self()<<"] error: "<<__func__
                 <<ex.GetString()
                 <<std::endl;
    }

    return ret;
}
