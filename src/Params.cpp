#include <sstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/regex/icu.hpp>
#include <boost/date_time.hpp>

#include <string>

#include "Params.h"
#include "GeoIPTools.h"
#include "Log.h"


Params::Params() :
    newClient(false),
    test_mode_(false),
    json_(false)
{
    time_ = boost::posix_time::second_clock::local_time();
}

/// IP посетителя.
Params &Params::ip(const std::string &ip)
{
    struct in_addr ipval;

    ip_ = ip;
    country_ = geoip->country_code_by_addr(ip_);
    region_ = geoip->region_code_by_addr(ip_);

    if(inet_pton(AF_INET, ip_.c_str(), &ipval))
    {
        key_long = ipval.s_addr;
    }
    else
    {
        key_long = 0;
    }

    return *this;
}
std::string time_t_to_string(time_t t)
{
    std::stringstream sstr;
    sstr << t;
    return sstr.str();
}

/// ID посетителя, взятый из cookie
Params &Params::cookie_id(const std::string &cookie_id)
{
    if(cookie_id.empty())
    {
        cookie_id_ = std::to_string(::time(NULL));
        newClient = true;
    }
    else
    {
        newClient = false;
    }

    cookie_id_ = cookie_id;
    key_long = key_long << 32;
    key_long = key_long | strtol(cookie_id_.c_str(),NULL,10);
    return *this;
}

/// ID информера.
Params &Params::informer_id(const std::string &informer_id)
{
    informer_id_ = informer_id;
    boost::to_lower(informer_id_);
    return *this;
}
/// Время. По умолчанию равно текущему моменту.
Params &Params::time(const boost::posix_time::ptime &time)
{
    time_ = time;
    return *this;
}

/** \brief  Двухбуквенный код страны посетителя.

    Если не задан, то страна будет определена по IP.

    Этот параметр используется служебными проверками работы информеров
    в разных странах и в обычной работе не должен устанавливаться.

    \see region()
    \see ip()
*/
Params &Params::country(const std::string &country)
{
    if(!country_.size())
    {
        country_ = country;
    }
    return *this;
}

/** \brief  Гео-политическая область в написании MaxMind GeoCity.

    Если не задана, то при необходимости будет определена по IP.

    Вместе с параметром country() используется служебными проверками
    работы информеров в разных странах и в обычной работе не должен
    устанавливаться.

    \see country()
    \see ip()
*/
Params &Params::region(const std::string &region)
{
    if(!region_.size())
    {
        region_ = region;
    }
    return *this;
}

/** \brief  Тестовый режим работы, в котором показы не записываются и переходы не записываються.

    По умолчанию равен false.
*/
Params &Params::test_mode(bool test_mode)
{
    test_mode_ = test_mode;
    return *this;
}


/// Выводить ли предложения в формате json.
Params &Params::json(bool json)
{
    json_ = json;
    return *this;
}

/// ID предложений, которые следует исключить из показа.
Params &Params::excluded_offers(const std::vector<std::string> &excluded)
{
    excluded_offers_ = excluded;
    return *this;
}

/** \brief  Виртуальный путь и имя вызываемого скрипта.

    Используется для построения ссылок на самого себя. Фактически,
    сюда должна передаваться сервреная переменная SCRIPT_NAME.
*/
Params &Params::script_name(const char *script_name)
{
    script_name_ = script_name? script_name : "";
    return *this;
}

/** \brief  Адрес страницы, на которой показывается информер.

    Обычно передаётся javascript загрузчиком.
*/
Params &Params::location(const char *location)
{
    return this->location(location? location : "");
}

/** \brief  Адрес страницы, на которой показывается информер.

    Обычно передаётся javascript загрузчиком.
*/
Params &Params::location(const std::string &location)
{
    location_ = location;
    return *this;
}

Params &Params::w(const std::string &w)
{
    w_ = w;
    return *this;
}

Params &Params::h(const std::string &h)
{
    h_ = h;
    return *this;
}


/**
 * строка, содержашяя контекст страницы
 */
Params &Params::context(const std::string &context)
{
    context_ = context;
    return *this;
}
Params &Params::context(const char *context)
{
    return this->context(context? context : "");
}
/**
 * строка, содержашяя поисковый запрос
 */
Params &Params::search(const std::string &search)
{
    search_ = search;
    return *this;
}

Params &Params::search(const char *search)
{
    return this->search(search? search : "");
}

std::string Params::getIP() const
{
    return ip_;
}
std::string Params::getCookieId() const
{
    return cookie_id_;
}

std::string Params::getUserKey() const
{
    return cookie_id_ + "-" + ip_;
}

unsigned long long Params::getUserKeyLong() const
{
    return key_long;
}

std::string Params::getCountry() const
{
    return country_;
}
std::string Params::getRegion() const
{
    return region_;
}

std::string Params::getInformerId() const
{
    return informer_id_;
}

boost::posix_time::ptime Params::getTime() const
{
    return time_;
}

bool Params::isTestMode() const
{
    return test_mode_;
}

bool Params::isJson() const
{
    return json_;
}

std::vector<std::string> Params::getExcludedOffers()
{
    return excluded_offers_;
}

std::string Params::getScriptName() const
{
    return script_name_;
}

std::string Params::getLocation() const
{
    return location_;
}

std::string Params::getW() const
{
    return w_;
}

std::string Params::getH() const
{
    return h_;
}

std::string Params::getContext() const
{
    return context_;
}

std::string Params::getSearch() const
{
    return search_;
}

std::string Params::getUrl() const
{
    std::stringstream url;
    url << script_name_ <<"?scr=" << informer_id_ <<"&show=json";

    if (test_mode_)
        url << "&test=true";
    if (!country_.empty())
        url << "&country=" << country_;
    if (!region_.empty())
        url << "&region=" << region_;
    url << "&";

    return url.str();
}

