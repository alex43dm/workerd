#include "GeoIPTools.h"

/** Возвращает объект GeoIP для определения страны по ip */
GeoIP *GeoCountry()
{
    static GeoIP *geo = 0;
    if (geo)
        return geo;
    geo = GeoIP_new(GEOIP_MEMORY_CACHE);
    return geo;
}

/** Возвращает объект GeoIP для определения города по ip.

    При первом вызове пытается открыть файл с базой данных, указанный в
    параметре ``filename``. Если ``filename`` не задан
 */
GeoIP *GeoCity(const char *filename)
{
    static GeoIP *geo = 0;
    static bool open_failed = false;

    if (open_failed)
        return 0;
    if (geo)
        return geo;

    if (filename)
        geo = GeoIP_open(filename, GEOIP_MEMORY_CACHE);
    else
        geo = GeoIP_open("/usr/share/GeoIP/GeoLiteCity.dat",
                         GEOIP_MEMORY_CACHE);
    open_failed = geo? false : true;
    return geo;
}

/** Возвращает двухбуквенный код страны по ``ip``.
    Если по какой-либо причине страну определить не удалось, возвращается
    пустая строка
*/
std::string country_code_by_addr(const std::string &ip)
{
    if (!GeoCountry())
        return "";

    const char *country = GeoIP_country_code_by_addr(GeoCountry(), ip.c_str());
    return country? country : "";
}


/** Возвращает название области по ``ip``.
    Если по какой-либо причине область определить не удалось, возвращается
    пустая строка.
*/
std::string region_code_by_addr(const std::string &ip)
{
    if (!GeoCity())
        return "";

    GeoIPRecord *record = GeoIP_record_by_addr(GeoCity(), ip.c_str());
    if (!record)
        return "";

    const char *region_name =
        GeoIP_region_name_by_code(record->country_code, record->region);
    return region_name? region_name : "";
}

std::string city_code_by_addr(const std::string &ip)
{
    if (!GeoCity())
        return "";

    GeoIPRecord *record = GeoIP_record_by_addr(GeoCity(), ip.c_str());
    if (!record)
        return "";

    return record->city ? record->city : "";
}
