#ifndef CORE_H
#define CORE_H

#include <list>
#include <vector>
#include <map>
#include <utility>

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
    std::string Process(const Params *params);

    void ProcessSaveResults();

    /** \brief  Адрес скрипта перехода на рекламное предложение.
     *
     * По умолчанию равен \c "/redirect", то есть скрипт будет указывать
     * на тот же сервер, который отдал информер.
     *
     * Примеры значений:
     *
     * - \code http://rg.yottos.com/redirect \endcode
     * - \code http://getmyad.vsrv-1.2.yottos.com/redirect \endcode
     * - \code http://rynok.yottos.com/Redirect.ashx \endcode
    */
    /*
    std::string redirect_script() const
    {
        return redirect_script_;
    }

    void set_redirect_script(const std::string &url)
    {
        redirect_script_ = url;
    }
*/
private:
    /// Время запуска службы
    boost::posix_time::ptime time_service_started_;
    /// Время начала последнего запроса
    boost::posix_time::ptime time_request_started_;
/*
    ///Адрес сервера
    std::string server_ip_;
    ///Скрипт перенаправления запроса при клике на рекламном предложении
    std::string redirect_script_;
*/
    /** \brief Удаление из вектора result баннеров, не подходящих по размеру для информера с идентификатором informer.
     *
     * @param result Вектор РП, которые нужно проверить на совместимость по размерам с инфомером informer.
     * @param informer идентификатор информера, для которого модуль формирует ответ.
     *
     * Добавлено RealInvest Soft.
     */
    //void filterOffersSize(std::map<long,Offer*> &result, const std::string& informerId);

    /** \brief Удаление из вектора result баннеров, не подходящих по размеру для информера informer.
     *
     * @param result Вектор РП, которые нужно проверить на совместимость по размерам с инфомером informer.
     * @param informer Информер, для которого модуль формирует ответ.
     *
     * Добавлено RealInvest Soft.
     */
    //void filterOffersSize(std::map<long,Offer*> &result, const Informer& informer);

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
    void RISAlgorithm(Offer::Vector &result);

    bool isSocial (Offer& i);

    DataBase *pDb;

    Kompex::SQLiteStatement *pStmtInformer, *pStmtOffer, *pStmtOfferDefault;

    pthread_t tid;

    Informer *informer;
    const Params *params;

    HistoryManager *hm;

    std::string pStmtOfferStr;

    bool all_social;

    int teasersCount;

    char *cmd;
    float teasersMediumRating;
    std::string tmpTableName;
    Offer::Vector result;
    Offer::Map items;
    Offer::Vector RISResult;

    bool getOffers( Offer::Map &result);
    Informer *getInformer();
    bool getAllOffers();

    /** \brief  Возвращает HTML для информера, содержащего предложения items */
    std::string OffersToHtml(const Offer::Vector &items, const std::string &url) const;
    /** \brief  Возвращает json-представление предложений ``items`` */
    std::string OffersToJson(const Offer::Vector &items) const;
    /** \brief  Возвращает безопасную json строку (экранирует недопустимые символы) */
    static std::string EscapeJson(const std::string &str);
};


#endif // CORE_H
