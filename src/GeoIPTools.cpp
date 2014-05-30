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
    if (!mGeoCountry)
        return "";

    const char *country = GeoIP_country_code_by_addr(mGeoCountry, ip.c_str());
    return country? country : "";
}


/** Возвращает название области по ``ip``.
    Если по какой-либо причине область определить не удалось, возвращается
    пустая строка.
*/
std::string GeoIPTools::region_code_by_addr(const std::string &ip) const
{
    if (!mGeoCity)
        return "";

    GeoIPRecord *record = GeoIP_record_by_addr(mGeoCity, ip.c_str());
    if (!record)
        return "";

    const char *region_name =
        GeoIP_region_name_by_code(record->country_code, record->region);

    std::string ret;
    ret = region_name? region_name : "";
    free((void*)region_name);

    return ret;
}

std::string GeoIPTools::city_code_by_addr(const std::string &ip) const
{
    if (!mGeoCity)
        return "";

    GeoIPRecord *record = GeoIP_record_by_addr(mGeoCity, ip.c_str());
    if (!record)
        return "";

    return record->city ? record->city : "";
}
