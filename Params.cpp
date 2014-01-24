#include <sstream>

#include <boost/algorithm/string/trim.hpp>
#include <boost/regex/icu.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>

#include <string>

#include "Params.h"
#include "utils/GeoIPTools.h"
#include "Log.h"


Params::Params() : test_mode_(false), json_(false)
{
    time_ = boost::posix_time::second_clock::local_time();

    replaceSymbol = boost::make_u32regex("[^а-яА-Яa-zA-Z0-9-]");
    replaceExtraSpace = boost::make_u32regex("\\s+");
    replaceNumber = boost::make_u32regex("(\\b)\\d+(\\b)");
}

/// IP посетителя.
Params &Params::ip(const std::string &ip)
{
    ip_ = ip;
    country_ = country_code_by_addr(ip_);
    region_ = region_code_by_addr(ip_);
    return *this;
}

/// ID посетителя, взятый из cookie
Params &Params::cookie_id(const std::string &cookie_id)
{
    cookie_id_ = cookie_id;
    return *this;
}
/// ID информера.
Params &Params::informer(const std::string &informer)
{
    informer_ = informer;
    boost::to_lower(informer_);
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
    country_ = country;
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
    region_ = region;
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

std::string Params::getCountry() const
{
    return country_;
}
std::string Params::getRegion() const
{
    return region_;
}

std::string Params::getInformer() const
{
    return informer_;
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
    url << script_name_ <<"?scr=" << informer_ <<"&show=json";

    if (test_mode_)
        url << "&test=true";
    if (!country_.empty())
        url << "&country=" << country_;
    if (!region_.empty())
        url << "&region=" << region_;
    url << "&";

    return url.str();
}

/**
      \brief Нормализирует строку строку.
  */
std::string Params::stringWrapper(const std::string &str, bool replaceNumbers)
{
    std::string t = str;
    //Заменяю все не буквы, не цифры, не минус на пробел
    t = boost::u32regex_replace(t,replaceSymbol," ");
    if (replaceNumbers)
    {
        //Заменяю отдельностояшие цифры на пробел, тоесть "у 32 п" замениться на
        //"у    п", а "АТ-23" останеться как "АТ-23"
        t = boost::u32regex_replace(t,replaceNumber," ");
    }
    //Заменяю дублируюшие пробелы на один пробел
    t = boost::u32regex_replace(t,replaceExtraSpace," ");
    boost::trim(t);
    return t;
}

std::string Params::getKeywordsString(const std::string& str)
{
    try
    {
        std::string q = str;
        boost::algorithm::trim(q);
        if (q.empty())
        {
            return std::string();
        }
        std::string qs  = stringWrapper(q, false);
        std::string qsn = stringWrapper(q, true);

        std::vector<std::string> strs;
        std::string exactly_phrases;
        std::string keywords;
        boost::split(strs,qs,boost::is_any_of("\t "),boost::token_compress_on);
        for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it)
        {
            exactly_phrases += "<<" + *it + " ";
            if (it != strs.begin())
            {
                keywords += " | " + *it;
            }
            else
            {
                keywords += " " + *it;
            }
        }
        std::string str = "((@exactly_phrases \"" + exactly_phrases + "\"~1) | (@title \"" + qsn + "\"/3)| (@description \"" + qsn + "\"/3) | (@keywords " + keywords + " ) | (@phrases \"" + qs + "\"~5))";
        return str;
    }
    catch (std::exception const &ex)
    {
        Log::err("exception %s: %s", typeid(ex).name(), ex.what());
        return std::string();
    }
}
std::string Params::getContextKeywordsString(const std::string& query)
{
    try
    {
        std::string q = query;
        boost::trim(q);
        if (q.empty())
        {
            return std::string();
        }
        std::string qs = stringWrapper(q);
        std::string qsn = stringWrapper(q, true);
        std::vector<std::string> strs;
        std::string exactly_phrases;
        std::string keywords;
        boost::split(strs,qs,boost::is_any_of("\t "),boost::token_compress_on);
        for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it)
        {
            exactly_phrases += "<<" + *it + " ";
            if (it != strs.begin())
            {
                keywords += " | " + *it;
            }
            else
            {
                keywords += " " + *it;
            }
        }
        std::string str = "((@exactly_phrases \"" + exactly_phrases + "\"~1) | (@title \"" + qsn + "\"/3)| (@description \"" + qsn + "\"/3) | (@keywords \"" + qs + "\"/3 ) | (@phrases \"" + qs + "\"~5))";
        return str;
    }
    catch (std::exception const &ex)
    {
        Log::err("exception %s: %s", typeid(ex).name(), ex.what());
        return std::string();
    }
}
