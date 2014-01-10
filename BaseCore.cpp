#include "DB.h"
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <ctime>
#include <cstdlib>
#include <sstream>

#include "Log.h"
#include "BaseCore.h"
#include "utils/base64.h"
#include "utils/benchmark.h"

#include "InformerTemplate.h"
#include "Informer.h"
#include "Campaign.h"
#include "Offer.h"
#include "Config.h"

BaseCore::BaseCore()
    : amqp_initialized_(false), amqp_down_(true), amqp_(0)
{
    LoadAllEntities();
    InitMessageQueue();
    InitMongoDB();
    time_service_started_ = boost::posix_time::second_clock::local_time();
}

BaseCore::~BaseCore()
{
    delete amqp_;
}

bool BaseCore::ProcessMQ()
{
    // Интервал (в секундах) между проверками MQ
    static int check_interval = 0;

    // При возникновении ошибки check_interval постепенно увеличивается до
    // max_check_interval с шагом interval_delta
    const int max_check_interval = 5 * 60;
    const int interval_delta = 5;

    if (!amqp_initialized_)
        return false;
    if (!time_last_mq_check_.is_not_a_date_time() &&
            ((boost::posix_time::second_clock::local_time() - time_last_mq_check_) <
             boost::posix_time::seconds(check_interval)))
        return false;
    time_last_mq_check_ = boost::posix_time::second_clock::local_time();
    if (amqp_down_)
        InitMessageQueue();

    try
    {
        {
            // Проверка сообщений advertise.#
            mq_campaign_->Get(AMQP_NOACK);
            AMQPMessage *m = mq_campaign_->getMessage();
            if (m->getMessageCount() > -1)
            {
                Log::info("Message retrieved:\nbody: %s\nrouting key:%sexchange:%s\n",
                m->getMessage(nullptr),m->getRoutingKey().c_str(),m->getExchange().c_str());
/*                Campaign campaign = Campaign(m->getMessage(nullptr));
                string logline = boost::str(
                                     boost::format("Message (key=%1%, body=%2%, "
                                                   "campaign=%3%)")
                                     % m->getRoutingKey()
                                     % m->getMessage(nullptr)
                                     % campaign.title());
                LogToAmqp(logline);
                //ReloadCampaign(campaign);*/
                time_last_mq_check_ = boost::posix_time::second_clock::local_time();
                check_interval = 2;
                return true;
            }
        }
        {
            // Проверка сообщений informer.#
            mq_informer_->Get(AMQP_NOACK);
            AMQPMessage *m = mq_informer_->getMessage();
            if (m->getMessageCount() > -1)
            {
                Log::info("Message retrieved:\nbody: %s\nrouting key:%sexchange:%s\n",
                m->getMessage(nullptr),m->getRoutingKey().c_str(),m->getExchange().c_str());
                /*
                Informer informer(m->getMessage(nullptr));
                string logline = boost::str(
                                     boost::format("Message (key=%1%, body=%2%, "
                                                   "informer=%3%)")
                                     % m->getRoutingKey()
                                     % m->getMessage(nullptr)
                                     % informer.title);
                LogToAmqp(logline);
                Benchmark bench("Informer reloaded");
//                Informer::loadInformer(informer, pDb->pDatabase);
                time_last_mq_check_ = second_clock::local_time();
                check_interval = 2;
                */
                return true;
            }
        }
        {
            // Проверка сообщений account.#
            mq_account_->Get(AMQP_NOACK);
            AMQPMessage *m = mq_account_->getMessage();
            if (m->getMessageCount() > -1)
            {
                Log::info("Message retrieved:\nbody: %s\nrouting key:%sexchange:%s\n",
                m->getMessage(nullptr),m->getRoutingKey().c_str(),m->getExchange().c_str());
                string logline = boost::str(
                                     boost::format("Message (key=%1%, body=%2%")
                                     % m->getRoutingKey()
                                     % m->getMessage(nullptr));
                LogToAmqp(logline);
                LoadAllEntities();
                time_last_mq_check_ = boost::posix_time::second_clock::local_time();
                check_interval = 2;
                return true;
            }
        }
        check_interval = 2;
        amqp_down_ = false;
    }
    catch (AMQPException &ex)
    {
        if (!amqp_down_)
            LogToAmqp("AMQP is down");
        amqp_down_ = true;
        if (check_interval + interval_delta < max_check_interval)
            check_interval += interval_delta;
        Log::err("AMQPException: %s", ex.getMessage().c_str());
    }
    return false;
}

/** Помещает сообщение \a message в журнал AMQP */
void BaseCore::LogToAmqp(const std::string &message)
{
    string logline = boost::str(
	    boost::format("%1% \t %2%")
	    % boost::posix_time::second_clock::local_time()
	    % message);
    mq_log_.push_back(logline);
}

/*
*  Загружает из основной базы данных следующие сущности:
*
*  - рекламные предложения;
*  - рекламные кампании;
*  - информеры.
*
*  Если в кампании нет рекламных предложений, она будет пропущена.
*/
void BaseCore::LoadAllEntities()
{
    if(Config::Instance()->pDb->reopen)
        return;

    Informer::loadAll(Config::Instance()->pDb->pDatabase);
    //LOG(INFO) << "Загрузили все информеры.\n";
    Campaign::loadAll(Config::Instance()->pDb->pDatabase);
    //LOG(INFO) << "Загрузили все кампании.\n";
    Offer::loadAll(Config::Instance()->pDb->pDatabase);
    //LOG(INFO) << "Загрузили все предложения.\n";

    Config::Instance()->pDb->postDataLoad();

    Config::Instance()->pDb->indexRebuild();
}


void BaseCore::ReloadAllEntities()
{
    Config::Instance()->pDb->postDataLoad();
}

/** \brief  Инициализация очереди сообщений (AMQP).

    Если во время инициализации произошла какая-либо ошибка, то сервис
    продолжит работу, но возможность оповещения об изменениях и горячего
    обновления будет отключена.
*/
void BaseCore::InitMessageQueue()
{
    try
    {
        // Объявляем точку обмена
        amqp_ = new AMQP("guest:guest@localhost//");
        exchange_ = amqp_->createExchange();
        exchange_->Declare("getmyad", "topic", AMQP_AUTODELETE);
        LogToAmqp("AMQP is up");

        // Составляем уникальные имена для очередей
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::string postfix = to_iso_string(now);
        boost::replace_first(postfix, ".", ",");
        std::string mq_advertise_name( "getmyad.advertise." + postfix );
        std::string mq_informer_name( "getmyad.informer." + postfix );
        std::string mq_account_name( "getmyad.account." + postfix );

        // Объявляем очереди
        mq_campaign_ = amqp_->createQueue();
        mq_campaign_->Declare(mq_advertise_name, AMQP_AUTODELETE | AMQP_EXCLUSIVE);
        mq_informer_ = amqp_->createQueue();
        mq_informer_->Declare(mq_informer_name, AMQP_AUTODELETE | AMQP_EXCLUSIVE);
        mq_account_ = amqp_->createQueue();
        mq_account_->Declare(mq_account_name, AMQP_AUTODELETE | AMQP_EXCLUSIVE);

        // Привязываем очереди
        exchange_->Bind(mq_advertise_name, "campaign.#");
        exchange_->Bind(mq_informer_name, "informer.#");
        exchange_->Bind(mq_account_name, "account.#");

        amqp_initialized_ = true;
        amqp_down_ = false;

        Log::info("Created ampq queues: %s, %s, %s",
                  mq_advertise_name.c_str(),
                  mq_informer_name.c_str(),
                  mq_account_name.c_str());
        LogToAmqp("Created amqp queue " + mq_advertise_name);
        LogToAmqp("Created amqp queue " + mq_informer_name);
        LogToAmqp("Created amqp queue " + mq_account_name);

    }
    catch (AMQPException &ex)
    {
        Log::err("Error in AMPQ init: %s, Feature will be disabled.", ex.getMessage().c_str());
        amqp_initialized_ = false;
        amqp_down_ = true;
    }
}

/** Подготовка базы данных MongoDB.
*/
void BaseCore::InitMongoDB()
{
    mongo::DB db("log");
    const int K = 1000;
    const int M = 1000 * K;
    db.createCollection("log.impressions", 600*M, true, 1*M);
}

/** Возвращает данные о состоянии службы
 *  TODO Надоб переписать с учётом использования boost::formater красивее будет как некак :)
 */
std::string BaseCore::Status()
{
     std::stringstream out;
    // Обработано запросов на момент прошлого обращения к статистике
//    static int last_time_request_processed = 0;

    // Время последнего обращения к статистике
    static boost::posix_time::ptime last_time_accessed;

    boost::posix_time::time_duration d;

    // Вычисляем количество запросов в секунду
    if (last_time_accessed.is_not_a_date_time())
        last_time_accessed = time_service_started_;
    //ptime now = microsec_clock::local_time();
  //  int millisecs_since_last_access =
    //    (now - last_time_accessed).total_milliseconds();
    //int millisecs_since_start =
    //    (now - time_service_started_).total_milliseconds();
    //int requests_per_second_current = 0;
    //int requests_per_second_average = 0;
    /*
    if (millisecs_since_last_access)
        requests_per_second_current =
            (request_processed_ - last_time_request_processed) * 1000 /
            millisecs_since_last_access;
    if (millisecs_since_start)
        requests_per_second_average = request_processed_ * 1000 /
                                      millisecs_since_start;

    last_time_accessed = now;
    last_time_request_processed = request_processed_;

    std::stringstream out;
    out << "<html>\n"
        "<head><meta http-equiv=\"content-type\" content=\"text/html; "
        "charset=UTF-8\">\n"
        "<style>\n"
        "body { font-family: Arial, Helvetica, sans-serif; }\n"
        "h1, h2, h3 {font-family: \"georgia\", serif; font-weight: 400;}\n"
        "table { border-collapse: collapse; border: 1px solid gray; }\n"
        "td { border: 1px dotted gray; padding: 5px; font-size: 10pt; }\n"
        "th {border: 1px solid gray; padding: 8px; font-size: 10pt; }\n"
        "</style>\n"
        "</head>"
        "<body>\n<h1>Состояние службы Yottos GetMyAd worker</h1>\n"
        "<table>"
        "<tr>"
        "<td>Обработано запросов:</td> <td><b>" << request_processed_ <<
        "</b> (" << requests_per_second_current << "/сек, "
        " в среднем " << requests_per_second_average << "/сек) "
        "</td></tr>\n"
        "<tr>"
        "<td>Общее кол-во показов:</td> <td><b>" << offer_processed_ <<
        "</b> (" << social_processed_ << " из них социальная реклама) "
        "</td></tr>\n";
*/
    out << "<tr><td>Имя сервера: </td> <td>" <<
        (getenv("SERVER_NAME")? getenv("SERVER_NAME"): "неизвестно") <<
        "</td></tr>\n";
        /*
    out << "<tr><td>IP сервера: </td> <td>" <<
        (server_ip_.empty()? "неизвестно": server_ip_) <<
        "</td></tr>\n";
        */
    out << "<tr><td>Текущее время: </td> <td>" <<
        boost::posix_time::second_clock::local_time() <<
        "</td></tr>\n";

    try
    {
        mongo::DB db_main;
        out << "<tr><td>Основная база данных:</td> <td>" <<
            db_main.server_host() << "/" << db_main.database() << "<br/>";
        out << "slave_ok = " << (db_main.slave_ok()? "true" : "false");
        if (db_main.replica_set().empty())
            out << " (no replica set)";
        else
            out << " (replSet=" << db_main.replica_set() << ")";
        out << "</td></tr>\n";
    }
    catch (mongo::DB::NotRegistered &)
    {
        out << "Основная база не определена</td></tr>\n";
    }
    /*
    out << "<tr><td>База данных Redis (краткосрочная история):</td> <td>" <<
        HistoryManager::instance()->getConnectionParams()->redis_short_term_history_host_ << ":";
    out << HistoryManager::instance()->getConnectionParams()->redis_short_term_history_port_;
    out << "(TTL =" << HistoryManager::instance()->getConnectionParams()->shortterm_expire_ << ")<br/>";
    out << "status = " << (HistoryManager::instance()->getDBStatus(1)? "true" : "false");
    out << "</td></tr>\n";

    out << "<tr><td>База данных Redis (долгосрочная история):</td> <td>" <<
        HistoryManager::instance()->getConnectionParams()->redis_long_term_history_host_ << ":";
    out << HistoryManager::instance()->getConnectionParams()->redis_long_term_history_port_ <<"<br/>";
    out << "status = " << (HistoryManager::instance()->getDBStatus(2)? "true" : "false");
    out << "</td></tr>\n";

    out << "<tr><td>База данных Redis (история показов):</td> <td>" <<
        HistoryManager::instance()->getConnectionParams()->redis_user_view_history_host_ << ":";
    out << HistoryManager::instance()->getConnectionParams()->redis_user_view_history_port_ ;
    out << "(TTL =" << HistoryManager::instance()->getConnectionParams()->views_expire_ << ")<br/>";
    out << "status = " << (HistoryManager::instance()->getDBStatus(3)? "true" : "false");
    out << "</td></tr>\n";

    out << "<tr><td>База данных Redis (констекст страниц):</td> <td>" <<
        HistoryManager::instance()->getConnectionParams()->redis_page_keywords_host_ << ":";
    out << HistoryManager::instance()->getConnectionParams()->redis_page_keywords_port_;
    out << "(TTL =" << HistoryManager::instance()->getConnectionParams()->context_expire_ << ")<br/>";
    out << "status = " << (HistoryManager::instance()->getDBStatus(4)? "true" : "false");
    out << "</td></tr>\n";

    out << "<tr><td>База данных Redis (категорий):</td> <td>" <<
        HistoryManager::instance()->getConnectionParams()->redis_category_host_ << ":";
    out << HistoryManager::instance()->getConnectionParams()->redis_category_port_<<"<br/>";;
    out << "status = " << (HistoryManager::instance()->getDBStatus(5)? "true" : "false");
    out << "</td></tr>\n";

    out << "<tr><td>База данных Redis (ретаргетинг):</td> <td>" <<
        HistoryManager::instance()->getConnectionParams()->redis_retargeting_host_ << ":";
    out << HistoryManager::instance()->getConnectionParams()->redis_retargeting_port_<<"<br/>";;
    out << "status = " << (HistoryManager::instance()->getDBStatus(6)? "true" : "false");
    out << "</td></tr>\n";
    */
    try
    {
        mongo::DB db_log("log");
        out << "<tr><td>База данных журналирования: </td> <td>" <<
            db_log.server_host() << "/" << db_log.database() << "<br/>";
        out << "slave_ok = " << (db_log.slave_ok()? "true" : "false");
        if (db_log.replica_set().empty())
            out << " (no replica set)";
        else
            out << " (replSet=" << db_log.replica_set() << ")";
        out << "</td></tr>";

    }
    catch (mongo::DB::NotRegistered &)
    {
        out << "База данных журналирования не определена</td></tr>\n";
    }

    out << "<tr><td>Время запуска:</td> <td>" << time_service_started_ <<
        "</td></tr>" <<
        "<tr><td>AMQP:</td><td>" <<
        (amqp_initialized_ && !amqp_down_? "активен" : "не активен") <<
        "</td></tr>\n";
    out <<  "<tr><td>Сборка: </td><td>" << __DATE__ << " " << __TIME__ <<
        "</td></tr>";
   // out <<  "<tr><td>Драйвер mongo: </td><td>" << mongo::versionString <<
       // "</td></tr>";
    out << "</table>\n";
/*
    std::list<Campaign> campaigns = Campaign::all();
    out << "<p>Загружено <b>" << campaigns.size() << "</b> кампаний: </p>\n";
    out << "<table><tr>\n"
        "<th>Наименование</th>"
        "<th>Действительна</th>"
        "<th>Социальная</th>"
        "<th>Предложений</th>"
        "</tr>\n";
        */
        /*
    for (auto it = campaigns.begin(); it != campaigns.end(); it++)
    {
        Campaign c(*it);
        out << "<tr>" <<
            "<td>" << c.title() << "</td>" <<
            "<td>" << (c.valid()? "Да" : "Нет") << "</td>" <<
            "<td>" << (c.social()? "Да" : "Нет") << "</td>" <<
            "<td>" << Offers::x()->offers_by_campaign(c).size() << "</td>"<<
            "</tr>\n";
    }*/
    out << "</table>\n\n";

    // Журнал сообщений AMQP
    out << "<p>Журнал AMQP: </p>\n"
        "<table>\n";
    int i = 0;
    for (auto it = mq_log_.begin(); it != mq_log_.end(); it++)
        out << "<tr><td>" << ++i << "</td>"
            "<td>" << *it << "</td></tr>\n";
    out << "<tr><td></td><td>Последняя проверка сообщений: " <<
        time_last_mq_check_ << "</td><tr>\n"
        "</table>\n";

    out << "</body>\n</html>\n";

    return out.str();
}
