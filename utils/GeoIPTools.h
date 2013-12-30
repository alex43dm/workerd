#ifndef GEOIPTOOLS_H
#define GEOIPTOOLS_H

#include <GeoIP.h>
#include <GeoIPCity.h>

#include <string>

/** Возвращает объект GeoIP для определения страны по ip */
GeoIP *GeoCountry();


/** Возвращает объект GeoIP для определения города по ip.

    При первом вызове пытается открыть файл с базой данных, указанный в
    параметре ``filename``. Если ``filename`` не задан
 */
GeoIP *GeoCity(const char *filename = 0);


/** Возвращает двухбуквенный код страны по ``ip``.
    Если по какой-либо причине страну определить не удалось, возвращается
    пустая строка
*/
std::string country_code_by_addr(const std::string &ip);


/** Возвращает название географической области по ``ip``.
    Если по какой-либо причине область определить не удалось, возвращается
    пустая строка.
*/
std::string region_code_by_addr(const std::string &ip);

std::string city_code_by_addr(const std::string &ip);

#endif // GEOIPTOOLS_H
