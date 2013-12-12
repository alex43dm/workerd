#include "DB.h"
#include "CampaignShowOptions.h"

using namespace mongo;
using namespace boost::posix_time;


/** Инициализация правила значениями по умолчанию */
CampaignShowOptions::CampaignShowOptions(Campaign c)
    : campaign_(c)
{
    reset();
}


/** Устанавливаем умолчания для правил */
void CampaignShowOptions::reset()
{
    loaded_ = false;
    show_coverage_ = Show_Allowed;
    impressions_per_day_limit_ = 0;
    start_show_time_ = end_show_time_ = time_duration(not_a_date_time);
    categories_.clear();

    using namespace boost::date_time;
    weekdays whole_week[7] = {Monday,
                              Tuesday,
                              Wednesday,
                              Thursday,
                              Friday,
                              Saturday,
                              Sunday
                             };
    days_of_week_.clear();
    for (int i = 0; i < 7; i++)
        days_of_week_.insert(whole_week[i]);
}


/** Загружает правила из базы данных */
bool CampaignShowOptions::load()
{
    /*
    reset();
    DB db;
    BSONObj adv = db.findOne("campaign", QUERY("guid" << campaign_.id()));
    BSONObj o = adv.getObjectField("showConditions");
    if (!o.isValid())
        return false;

    // Ограничение на показ в выбранных странах
    BSONObjIterator it = o.getObjectField("geoTargeting");
    while (it.more())
        country_targeting_.insert(it.next().str());

    // Ограничение на показ в выбранных областях
    it = o.getObjectField("regionTargeting");
    while (it.more())
        region_targeting_.insert(it.next().str());

    // Дни недели, в которые осуществляется показ
    it = o.getObjectField("daysOfWeek");
    if (it.more())
        days_of_week_.clear();
    while (it.more())
    {
        int day = it.next().numberInt();
        days_of_week_.insert(boost::date_time::weekdays(day));
    }

    // Ограничение на кол-во показов в сутки
    BSONElement el = o.getField("impressionsPerDayLimit");
    impressions_per_day_limit_ = el.numberInt();

    // Время старта и остановки кампании
    start_show_time_ = time_duration(
                           DB::toInt(o.getFieldDotted("startShowTime.hours")),
                           DB::toInt(o.getFieldDotted("startShowTime.minutes")), 0);
    end_show_time_ = time_duration(
                         DB::toInt(o.getFieldDotted("endShowTime.hours")),
                         DB::toInt(o.getFieldDotted("endShowTime.minutes")), 0);

    // Показывать кампанию на всех информерах, тематических или явно выбранных
    string o_show_coverage = o.getStringField("showCoverage");
    if (o_show_coverage == "all")
        show_coverage_ = Show_All;
    else if (o_show_coverage == "thematic")
        show_coverage_ = Show_Thematic;
    else if (o_show_coverage == "allowed")
        show_coverage_ = Show_Allowed;
    else
        show_coverage_ = Show_Allowed;

    // Множества информеров, аккаунтов и доменов, на которых разрешён показ
    it = o.getObjectField("allowed").getObjectField("informers");
    while (it.more())
        allowed_informers_.insert(it.next().str());
    it = o.getObjectField("allowed").getObjectField("accounts");
    while (it.more())
        allowed_accounts_.insert(it.next().str());
    it = o.getObjectField("allowed").getObjectField("domains");
    while (it.more())
        allowed_domains_.insert(it.next().str());

    // Множества информеров, аккаунтов и доменов, на которых запрещён показ
    it = o.getObjectField("ignored").getObjectField("informers");
    while (it.more())
        ignored_informers_.insert(Informer(it.next().str()));
    it = o.getObjectField("ignored").getObjectField("accounts");
    while (it.more())
        ignored_accounts_.insert(it.next().str());
    it = o.getObjectField("ignored").getObjectField("domains");
    while (it.more())
        ignored_domains_.insert(it.next().str());

    // Тематические категории, к которым относится кампания
    it = o.getObjectField("categories");
    while (it.more())
        categories_.insert(it.next().str());
*/
    loaded_ = true;
    return true;
}

// ----------------------------------------------------------------------------

std::map<Campaign, CampaignShowOptions *> CampaignOptionsCache::cache_;

CampaignShowOptions *CampaignOptionsCache::get(const Campaign &c)
{
    CampaignShowOptions *o;
    o = cache_[c];
    if (o)
        return o;
    o = new CampaignShowOptions(c);
    o->load();
    cache_[c] = o;
    return o;
}


void CampaignOptionsCache::invalidate()
{
    for (auto it = cache_.begin(); it != cache_.end(); it++)
    {
        delete it->second;
    }
    cache_.clear();
}
