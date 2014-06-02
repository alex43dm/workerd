#include "Config.h"
#include "GeoIPTools.h"

GeoIPTools* GeoIPTools::mInstance = NULL;

GeoIPTools::GeoIPTools()
{
    mGeoCountry = GeoIP_new(GEOIP_MEMORY_CACHE);
    mGeoCity = GeoIP_open(cfg->geocity_path_.c_str(), GEOIP_MEMORY_CACHE);
}

GeoIPTools::~GeoIPTools()
{
    GeoIP_delete(mGeoCountry);
    GeoIP_delete(mGeoCity);
}

GeoIPTools* GeoIPTools::Instance()
{
    if (!mInstance)   // Only allow one instance of class to be generated.
        mInstance = new GeoIPTools();

    return mInstance;
}


/** Возвращает двухбуквенный код страны по ``ip``.
    Если по какой-либо причине страну определить не удалось, возвращается
    пустая строка
*/
std::string GeoIPTools::country_code_by_addr(const std::string &ip) const
{
    std::string ret;
    const char *country;

    if (!mGeoCountry)
        return ret;

    if((country = GeoIP_country_code_by_addr(mGeoCountry, ip.c_str())))
    {
        ret = country;
        free((void*)country);
    }

    return ret;
}


/** Возвращает название области по ``ip``.
    Если по какой-либо причине область определить не удалось, возвращается
    пустая строка.
*/
std::string GeoIPTools::region_code_by_addr(const std::string &ip) const
{
    std::string ret;
    const char *region_name;
    GeoIPRecord *record;

    if (!mGeoCity)
        return ret;

    if (!(record = GeoIP_record_by_addr(mGeoCity, ip.c_str())))
    {
        return ret;
    }
    else
    {
        if((region_name =
            GeoIP_region_name_by_code(record->country_code, record->region)))
        {
            ret = region_name;
            free((void*)region_name);
        }

        GeoIPRecord_delete(record);
    }

    return ret;
}

std::string GeoIPTools::city_code_by_addr(const std::string &ip) const
{
    std::string ret;
    GeoIPRecord *record;

    if (!mGeoCity)
        return ret;

    if((record = GeoIP_record_by_addr(mGeoCity, ip.c_str())))
    {
        ret = record->city;
        GeoIPRecord_delete(record);
    }

    return ret;
}
