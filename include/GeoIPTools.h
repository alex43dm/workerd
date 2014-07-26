#ifndef GEOIPTOOLS_H
#define GEOIPTOOLS_H

#include <GeoIP.h>
#include <GeoIPCity.h>

#include <string>

class GeoIPTools
{
public:
    static GeoIPTools* Instance();
    virtual ~GeoIPTools();
/** Возвращает двухбуквенный код страны по ``ip``.
    Если по какой-либо причине страну определить не удалось, возвращается
    пустая строка
*/

std::string country_code_by_addr(const std::string &ip) const;


/** Возвращает название географической области по ``ip``.
    Если по какой-либо причине область определить не удалось, возвращается
    пустая строка.
*/
std::string region_code_by_addr(const std::string &ip) const;

std::string city_code_by_addr(const std::string &ip) const;


private:
    GeoIP *mGeoCity, *mGeoCountry;
    static GeoIPTools *mInstance;
    GeoIPTools();
};

extern GeoIPTools *geoip;

#endif // GEOIPTOOLS_H
