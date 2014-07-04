#ifndef CORE_H
#define CORE_H

#include <list>
#include <vector>
//#include <map>
#include <utility>
#include <algorithm>
#include <set>

#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>

#include "Offer.h"
#include "Informer.h"
#include "Params.h"
#include "DataBase.h"
#ifndef DUMMY
#include "HistoryManager.h"
#endif

#include "Core_DataBase.h"

/// Класс, который связывает воедино все части системы.
class Core : public Core_DataBase
{
public:
    Core();
    ~Core();

    /** \brief  Обработка запроса на показ рекламы.
     * Самый главный метод. Возвращает HTML-строку, которую нужно вернуть
     * пользователю.
     */
    std::string Process(Params *params);

    void ProcessSaveResults();

private:
    boost::posix_time::ptime
    /// Время запуска службы
    time_service_started_,
    /// Время начала последнего запроса
    time_request_started_,
    ///start and ent time core process
    startCoreTime, endCoreTime;
    ///core thread id
    pthread_t tid;
    ///parameters to process: from http GET
    Params *params;
#ifndef DUMMY
    ///history manager
    HistoryManager *hm;
#endif
    ///retargeting offers count
    unsigned RetargetingCount;
    ///campaigns to show
    std::multiset<unsigned long long> OutPutCampaignSet;
    ///result offers vector
    Offer::Vector vOutPut;
    ///all offers to show
    Offer::Map items;

    /** \brief Основной алгоритм отбора РП RealInvest Soft. */
    void RISAlgorithm(const Offer::Map &items, Offer::Vector &RISresult);
    void RISAlgorithmRetagreting(const Offer::Vector &result, Offer::Vector &RISResult);
    //bool contains( const Offer::Vector &v, const Offer::itV p) const {return std::find(v.begin(), v.end(), *p) != v.end();}
    //bool containsInRetargeting(const Offer::itV p)const {return contains(resultRetargeting, p);}
    /** \brief  Возвращает HTML для информера, содержащего предложения items */
    std::string OffersToHtml(const Offer::Vector &items, const std::string &url) const;
    /** \brief  Возвращает json-представление предложений ``items`` */
    std::string OffersToJson(const Offer::Vector &items) const;
    /** \brief  Возвращает безопасную json строку (экранирует недопустимые символы) */
    static std::string EscapeJson(const std::string &str);
    /** \brief logging Core result in syslog */
    void log();
};

#endif // CORE_H
