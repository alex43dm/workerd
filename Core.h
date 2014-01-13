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
     *
     * Самый главный метод. Возвращает HTML-строку, которую нужно вернуть
     * пользователю.
     *
     * Пример вызова:
     *
     * \Example
     * \code
     * Core core(Params().ip("192.168.0.1")
     *                         .informer("informer#01"));
     * \endcode
     *
     * \param params    Параметры запроса.
     */
    std::string Process(const Params &params, Offer::Map&);

    void ProcessSaveResults(const Params &params, const Offer::Map &items);

    bool getOffers(const Params &params, Offer::Map &result);

    Informer *getInformer(const Params &params);


    /** \brief  Новый алгоритм. Добавлен RealInvest Soft.
     *
     * Возвращает РП, отобранные для показа по новому алгоритму RISAlgorithm с учетом рейтингов РП внутри рекламных блоков
     *
     * \param offersIds    Список пар (идентификатор, вес), где идентификатор - это идентификатор РП, отобранного поиском по индексу, вес - значение соответствия РП с данным идентификатором запросу (в алгоритме при вычислении веса вес, который вернула CLucene умножается на вес, задаваемый в настройках).
     * \param params    Параметры запроса.
     * \param camps    Список кампаний, по которым шёл выбор offersIds.
     *
     * \see RISAlgorithm
     * \see createVectorOffersByIds
     */
    Offer::Map getOffersRIS(const std::list<std::pair<std::pair<std::string, float>,
                                    std::pair<std::string, std::pair<std::string, std::string>>>> &offersIds,
                                    const Params &params, const std::list<Campaign> &camps,
                                    bool &clean, bool &updateShort, bool &updateContext);




    /** \brief  Увеличивает счётчики показов предложений ``items`` */
    void markAsShown(const Offer::Map &,const Params &params);

    /** \brief  Возвращает HTML для информера, содержащего предложения items */
    std::string OffersToHtml(const Offer::Map &items, const std::string &url) const;

    /** \brief  Возвращает json-представление предложений ``items`` */
    std::string OffersToJson(const Offer::Map &items) const;

    /** \brief  Возвращает безопасную json строку (экранирует недопустимые символы) */
    static std::string EscapeJson(const std::string &str);

    /** \brief  IP сервера, на котором запущена служба */
    std::string server_ip() const
    {
        return server_ip_;
    }

    void set_server_ip(const std::string &ip)
    {
        server_ip_ = ip;
    }

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
    std::string redirect_script() const
    {
        return redirect_script_;
    }

    void set_redirect_script(const std::string &url)
    {
        redirect_script_ = url;
    }

private:
    /// Счётчик обработанных запросов
    static int request_processed_;

    static int offer_processed_;

    static int social_processed_;

    /// Время запуска службы
    boost::posix_time::ptime time_service_started_;


    /// Время начала последнего запроса
    boost::posix_time::ptime time_request_started_;

    ///Адрес сервера
    std::string server_ip_;

    ///Скрипт перенаправления запроса при клике на рекламном предложении
    std::string redirect_script_;

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
    void RISAlgorithm(Offer::Map &result, const Params &params);

    bool isSocial (Offer& i);

    DataBase *pDb;

    Kompex::SQLiteStatement *pStmtInformer, *pStmtOffer, *pStmtOfferDefault;

    pthread_t tid;

    Informer *informer;

    HistoryManager *hm;

    std::string pStmtOfferStr;

    bool all_social;

    int teasersCount;

    char *cmd;
    float teasersMediumRating;
    std::string tmpTableName;
};


#endif // CORE_H
