#include "DB.h"
#include <iostream>
#include "Log.h"
#include <cmath>
#include <map>
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/gregorian_calendar.hpp>

#include "Rules.h"
#include "utils/GeoIPTools.h"

using namespace mongo;
using boost::lexical_cast;
using namespace boost::posix_time;
using std::list;
using std::string;
using std::map;

Rules::Rules()
    : region_loaded_(false), time_(not_a_date_time)
{
}

Rules::~Rules()
{
}



/** Возвращает список рекламных кампаний, которые следует отображать при
    заданных условиях */
list<Campaign> Rules::campaigns()
{
    // Получаем список всех подходящих кампаний
    list<Campaign> result;
    list<Campaign> all = Campaign::all();
    remove_copy_if(all.begin(), all.end(), back_inserter(result),
                   !boost::bind(&Rules::is_campaign_valid, this, _1));

    return result;
}
/** Возвращает список не социальных рекламных кампаний, которые следует отображать при
    заданных условиях */
list<Campaign> Rules::campaignsNotSocial()
{
    // Получаем список всех подходящих кампаний
    list<Campaign> result;
    list<Campaign> all = Campaign::all();
    remove_copy_if(all.begin(), all.end(), back_inserter(result),
                   !boost::bind(&Rules::is_campaign_valid, this, _1));

    list<Campaign>::iterator new_end =
        remove_if(result.begin(), result.end(),
                  boost::bind(&Rules::is_campaign_social, this, _1));
    result.erase(new_end, result.end());

    return result;
}
/** Возвращает список социальных рекламных кампаний, которые следует отображать при
    заданных условиях */
list<Campaign> Rules::campaignsSocial()
{
    // Получаем список всех подходящих кампаний
    list<Campaign> result;
    list<Campaign> all = Campaign::all();
    remove_copy_if(all.begin(), all.end(), back_inserter(result),
                   !boost::bind(&Rules::is_campaign_valid, this, _1));

    list<Campaign>::iterator new_end =
        remove_if(result.begin(), result.end(),
                  !boost::bind(&Rules::is_campaign_social, this, _1));
    result.erase(new_end, result.end());

    return result;
}


/** Проверяет кампанию c на соответствие заданным условиям */
bool Rules::is_campaign_valid(const Campaign &c) const
{
    //return true;
    CampaignShowOptions *o = CampaignOptionsCache::get(c);

    // Для заблокированных информеров не подходят коммерческие кампании
    if (informer_.valid && informer_.blocked && !c.social())
    {
        return false;
    }


    // Таргетинг по странам
    if (!o->country_targeting().empty() &&
            !o->country_targeting().count(country_))
        return false;


    // День недели
    if (!time_.is_not_a_date_time())
    {
        boost::gregorian::greg_weekday wd = time_.date().day_of_week();
        if (!o->days_of_week().count(wd.as_enum()))
            return false;
    }


    // Время просмотра
    if (!time_.is_not_a_date_time())
    {
        time_duration t = time_.time_of_day();
        time_duration s = o->start_show_time();
        time_duration e = o->end_show_time();
        time_duration zero(0,0,0);
        if (!s.is_not_a_date_time() && s != zero && t < s)
            return false;
        if (!e.is_not_a_date_time() && e != zero && t > e)
            return false;
    }
/*
    // Разрешённые/запрещённые информеры и тематики
    if (!informer_.is_null())
    {
        // Если у кампании стоит отметка "Показывать на всех информерах",
        // то проверяем исключения (ignored informers, domains, accounts).
        if (o->show_coverage() == CampaignShowOptions::Show_All)
        {
            if (contains(o->ignored_accounts, informer_.user) ||
                    contains(o->ignored_domains, informer_.domain) ||
                    contains(o->ignored_informers, informer_))
                return false;
        }
        // Если стоит отметка "Показывать на разрешённых, то проверяем на
        // попадание в разрешённые информеры/домены/аккаунты
        else if (o->show_coverage() == CampaignShowOptions::Show_Allowed)
        {
            if (!contains(o->allowed_accounts(), informer_.user()) &&
                    !contains(o->allowed_domains(), informer_.domain()) &&
                    !contains(o->allowed_informers(), informer_))
                return false;
        }


        // Тематическое покрытие
        else if (o->show_coverage() == CampaignShowOptions::Show_Thematic)
        {
            // Если есть в игнорируемых, кампания не подходит
            if (contains(o->ignored_accounts(), informer_.user()) ||
                    contains(o->ignored_domains(), informer_.domain()) ||
                    contains(o->ignored_informers(), informer_))
                return false;
            // Если есть в разрешённых, то дальнейший тест на тематики
            // пропускается, кампания считается годной
            if (!contains(o->allowed_accounts(), informer_.user()) &&
                    !contains(o->allowed_domains(), informer_.domain()) &&
                    !contains(o->allowed_informers(), informer_))
            {
                // Проверяем соответствие тематик кампании и информера
                set<string> intersection;
                set_intersection(o->categories().begin(),
                                 o->categories().end(),
                                 informer_.categories().begin(),
                                 informer_.categories().end(),
                                 inserter(intersection, intersection.begin()));
                if (intersection.empty())
                {
                    return false;
                }
            }
        }
    }
*/
    // Таргетинг по областям
    if (!o->region_targeting().empty() &&!o->region_targeting().count(region()) )
    {
        return false;
    }

    return true;
}


/** Является ли кампания социальной */
bool Rules::is_campaign_social(const Campaign &c) const
{
    return c.social();
}


/** Устанавливает ip пользователя, при этом пытается определить страну.
    Определение области откладывается до того момента, когда это действительно
    понадобится. */
void Rules::set_ip(const std::string &ip)
{
    ip_ = ip;
    set_country(country_code_by_addr(ip));
    region_loaded_ = false;
}


/** Возвращает область пользователя.
    При необходимости происходит определение (только первый вызов) */
std::string Rules::region() const
{
    if (!region_loaded_)
    {
        region_ = region_code_by_addr(ip_);
        region_loaded_ = true;
    }
    return region_;
}

/** Явное задание области, которое перекрывает автоматическое определение */
void Rules::set_region(const std::string &region)
{
    region_ = region;
    region_loaded_ = true;
}
