#ifndef BASECORE_H
#define BASECORE_H

#include "../config.h"

#include <boost/date_time.hpp>

#ifndef AMQPCPP_OLD
#include <AMQPcpp.h>
#else
#include <amqpcpp.h>
#endif


#include "DataBase.h"
#include "ParentDB.h"

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

private:
    void InitMessageQueue();

 /// Время запуска службы
    boost::posix_time::ptime time_service_started_,time_mq_check_;

    AMQP *amqp_;

    /// Точка обмена
    AMQPExchange *exchange_;
    /// Очередь сообщений об изменениях в кампаниях
    AMQPQueue *mq_campaign_;

    /// Очередь сообщений об изменениях в информерах
    AMQPQueue *mq_informer_;

   /// Очередь сообщений об изменениях в offer
    AMQPQueue *mq_advertise_;
    /// Очередь сообщений об изменениях в конфигурации
    //AMQPQueue *mq_process_;

    std::string toString(AMQPMessage *m);
    bool cmdParser(const std::string &cmd, std::string &offerId, std::string &campaignId);
    ParentDB *pdb;
    std::string mq_log_;
};


#endif // CORE_H
