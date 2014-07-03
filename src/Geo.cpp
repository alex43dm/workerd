#include "Geo.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "Config.h"

Geo::Geo(char *cmd, size_t len):
    cmd(cmd),
    len(len)
{
}

Geo::~Geo()
{
}

std::string Geo::compute(const std::string &country, const std::string &region)
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
            }
        }
        else if(country.size())
        {

            geo =
                "INNER JOIN geoTargeting AS geo INDEXED BY idx_geoTargeting_id_geo_id_cam ON geo.id_cam=cn.id INNER JOIN GeoLiteCity AS reg INDEXED BY idx_GeoRerions_country_city ON geo.id_geo = reg.locId AND((reg.country='"+country+"' OR reg.country='O1') AND (reg.city='' OR reg.city='NOT FOUND'))";
        }
    }
    return geo;
}
