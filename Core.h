#ifndef CORE_H
#define CORE_H

#include <list>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>

#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>

#include "Offer.h"
#include "Informer.h"
#include "Params.h"
#include "DataBase.h"
#include "HistoryManager.h"

/// Класс, который связывает воедино все части системы.
class Core
{
public:
    /** \brief  Создаёт ядро.
     *
     * Производит все необходимые инициализации:
     *
     * - Загружает все сущности из базы данных;
     * - Подключается к RabbitMQ и создаёт очереди сообщений;
     * - При необходимости создаёт локальную базу данных MongoDB с нужными
     *   параметрами.
     */
    Core();

    /** Пытается красиво закрыть очереди RabbitMQ, но при работе с FastCGI
     *  никогда не вызывается (как правило, процессы просто снимаются).
     */
    ~Core();

    /** \brief  Обработка запроса на показ рекламы.
     * Самый главный метод. Возвращает HTML-строку, которую нужно вернуть
     * пользователю.
     */
    std::string Process(Params *params);

    void ProcessSaveResults();

private:
    /// Время запуска службы
    boost::posix_time::ptime time_service_started_;
    /// Время начала последнего запроса
    boost::posix_time::ptime time_request_started_;
    /** \brief Проверка размеров РП.
      *
      * @param offer РП, которое нужно проверить по размеру.
      * @param informer Информер, для которого модуль формирует ответ.
      *
      * Добавлено RealInvest Soft.
      *
      * Если РП offer является баннером и его размеры не равны размерам РБ informer, возвращает false. Иначе - true.
      */
    bool checkBannerSize(const Offer *offer);

    /** \brief Основной алгоритм отбора РП RealInvest Soft. */
    void RISAlgorithm(Offer::Vector &result, Offer::Vector &RISresult, unsigned outLen);

    bool isSocial (Offer& i);

    DataBase *pDb;

    Kompex::SQLiteStatement *pStmtInformer, *pStmtOffer, *pStmtOfferDefault;

    pthread_t tid;

    Informer *informer;
    Params *params;

    HistoryManager *hm;

    std::string pStmtOfferStr,
                InformerStr;

    bool all_social;

    unsigned teasersCount;

    char *cmd;
    float teasersMediumRating;
    std::string tmpTableName;
    Offer::Vector vOutPut;
    Offer::Vector result, resultRetargeting;
    Offer::Map items;

    bool getOffers(Offer::Map &result);
    Informer *getInformer();
    bool getAllOffers(Offer::Map &v);
    bool getAllRetargeting(Offer::Vector &v);
    void RISAlgorithmRetagreting(Offer::Vector &result, Offer::Vector &RISResult, unsigned outLen);

    //bool contains( const Offer::Vector &v, const Offer::itV p) const {return std::find(v.begin(), v.end(), *p) != v.end();}
    //bool containsInRetargeting(const Offer::itV p)const {return contains(resultRetargeting, p);}

    /** \brief  Возвращает HTML для информера, содержащего предложения items */
    std::string OffersToHtml(const Offer::Vector &items, const std::string &url) const;
    /** \brief  Возвращает json-представление предложений ``items`` */
    std::string OffersToJson(const Offer::Vector &items) const;
    /** \brief  Возвращает безопасную json строку (экранирует недопустимые символы) */
    static std::string EscapeJson(const std::string &str);
    std::string getGeo();
};

#endif // CORE_H
