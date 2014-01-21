#ifndef BASECORE_H
#define BASECORE_H

#include <list>
#include <vector>
#include <map>
#include <utility>
#include <boost/date_time.hpp>
#include <AMQPcpp.h>

#include "DataBase.h"

/// Класс, который связывает воедино все части системы.
class BaseCore
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
    BaseCore();

    /** Пытается красиво закрыть очереди RabbitMQ, но при работе с FastCGI
     *  никогда не вызывается (как правило, процессы просто снимаются).
     */
    ~BaseCore();


    /** \brief  Загружает все сущности, которые используются при показе
     *          рекламы. */
    void LoadAllEntities();


    void ReloadAllEntities();

    /** \brief  Обрабатывает новые сообщения в очереди RabbitMQ. */
    bool ProcessMQ();

    /** \brief  Выводит состояние службы и некоторую статистику */
    std::string Status();


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

private:
    void InitMessageQueue();
    void InitMongoDB();
    void LogToAmqp(const std::string &message);

 /// Время запуска службы
    boost::posix_time::ptime time_service_started_;

    AMQP *amqp_;

    /// Точка обмена
    AMQPExchange *exchange_;
    /// Очередь сообщений об изменениях в кампаниях
    AMQPQueue *mq_campaign_;

    /// Очередь сообщений об изменениях в информерах
    AMQPQueue *mq_informer_;

   /// Очередь сообщений об изменениях в offer
    AMQPQueue *mq_advertise_;

    /// История полученных сообщений MQ
    std::vector<std::string> mq_log_;
    std::string toString(AMQPMessage *m);
    bool cmdParser(const std::string &cmd, std::string &offerId, std::string &campaignId);
};


#endif // CORE_H
