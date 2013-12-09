#ifndef RULES_H
#define RULES_H

#include <list>
#include <set>
#include <map>
#include <string>
#include <boost/date_time.hpp>

#include "Informer.h"
#include "Campaign.h"
#include "CampaignShowOptions.h"
#include "Offer.h"

/**
	\brief Класс, предназначенный для отбора рекламных кампаний, удовлетворяющих заданным параметрам
*/
class Rules
{
public:
    Rules();
    ~Rules();


    /** Возвращает список рекламных кампаний, которые нужно отображать
    при данных условиях */
    std::list<Campaign> campaigns();

    /** Возвращает список не социальных рекламных кампаний, которые нужно отображать
    при данных условиях */
    std::list<Campaign> campaignsNotSocial();

    /** Возвращает список социальных рекламных кампаний, которые нужно отображать
    при данных условиях */
    std::list<Campaign> campaignsSocial();

    /** IP пользователя.

    При задании IP будет предпринята попытка определить страну и область
    (см. ``country`` и ``region``). */
    std::string ip() const
    {
        return ip_;
    }
    void set_ip(const std::string &ip);

    /** Страна пользователя, представленная двухбуквенным кодом
    (например, "UA", "RU", "BY").

    При установке ip пользователя (``set_ip()``) страна будет определена
    автоматически. Если страну определить не удалось, свойство будет
    равно пустой строке. Явное задание страны (``set_country``) после
    вызова ``set_ip`` перекрывает автоматически определённую страну. */
    std::string country() const
    {
        return country_;
    }
    void set_country(const std::string &country)
    {
        country_ = country;
    }

    /** Географическая область, которой принадлежит ip.

    Используется написание области, принятое в базе данных MaxMind GeoCity,
    например, "Kharkivs'ka Oblast'".

    При установке ip пользователя (``set_ip()``) область будет определена
    автоматически. Если область определить не удалось, свойство будет
    равно пустой строке. Явное задание области (``set_region``) после
    вызова ``set_ip`` перекрывает автоматически определённую область. */
    std::string region() const;
    void set_region(const std::string &region);

    /** Информер, на котором собирается показываться реклама.
    Если не задан, то проверки на информеры пропускаются */
    Informer informer() const
    {
        return informer_;
    }
    void set_informer(const Informer &informer)
    {
        informer_ = informer;
    }

    /** Время, для которого принимается решение.
    Если не задано, то проверка на время и дни недели пропускается. */
    boost::posix_time::ptime time() const
    {
        return time_;
    }
    void set_time(const boost::posix_time::ptime &time)
    {
        time_ = time;
    }


private:
    std::string ip_;
    std::string country_;
    mutable std::string region_;
    mutable bool region_loaded_;	// Произведено ли определение области
    Informer informer_;
    boost::posix_time::ptime time_;

    /** Возвращат true, если кампания c подходит под заданные параметры */
    bool is_campaign_valid(const Campaign &c) const;

    /** Возвращает true, если кампания c является социальной */
    bool is_campaign_social(const Campaign &c) const;

    /** Возвращает true, если множество set содержит element */
    template<class E>
    bool contains(const std::set<E> &set, const E &element) const
    {
        return set.find(element) != set.end();
    }

};



#endif // RULES_H
