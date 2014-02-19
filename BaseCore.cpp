#include "DB.h"
#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
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
{
    time_service_started_ = boost::posix_time::second_clock::local_time();
    pdb = new ParentDB();
    LoadAllEntities();
    InitMessageQueue();
    InitMongoDB();
}

BaseCore::~BaseCore()
{
    delete amqp_;
}
std::string BaseCore::toString(AMQPMessage *m)
{
    unsigned len;
    char *pMes;

#ifdef AMQPCPP_OLD
    pMes = m->getMessage();
    len = strlen(pMes);
#else
    pMes = m->getMessage(&len);
#endif // AMQPCPP_OLD

    return std::string(pMes,len);
}

bool BaseCore::ProcessMQ()
{
    try
    {
        {
            // Проверка сообщений campaign.#
            mq_campaign_->Get(AMQP_NOACK);
            AMQPMessage *m = mq_campaign_->getMessage();
            if (m->getMessageCount() > -1)
            {
                Log::gdb("BaseCore::ProcessMQ campaign: %s",m->getRoutingKey().c_str());
                if(m->getRoutingKey() == "campaign.update")
                {
                    Campaign::update(Config::Instance()->pDb->pDatabase, toString(m));
                }
                else if(m->getRoutingKey() == "campaign.delete")
                {
                    Campaign::remove(Config::Instance()->pDb->pDatabase, toString(m));
                }
                else if(m->getRoutingKey() == "campaign.start")
                {
                    Campaign::startStop(Config::Instance()->pDb->pDatabase, toString(m), 1);
                }
                else if(m->getRoutingKey() == "campaign.stop")
                {
                    Campaign::startStop(Config::Instance()->pDb->pDatabase, toString(m), 0);
                }
                return true;
            }
        }
        {
            // Проверка сообщений advertise.#
            std::string m1, ofrId, cmgId;
            mq_advertise_->Get(AMQP_NOACK);
            AMQPMessage *m = mq_advertise_->getMessage();
            if (m->getMessageCount() > -1)
            {
                Log::gdb("BaseCore::ProcessMQ advertise: %s",m->getRoutingKey().c_str());
                m1 = toString(m);
                if(m->getRoutingKey() == "advertise.update")
                {
                    if(cmdParser(m1,ofrId,cmgId))
                    {
                        pdb->OfferLoad(QUERY("guid" << ofrId));
                    }
                }
                else if(m->getRoutingKey() == "advertise.delete")
                {
                    if(cmdParser(m1,ofrId,cmgId))
                    {
                        pdb->OfferRemove(ofrId);
                    }
                }

                return true;
            }
        }
        {
            // Проверка сообщений informer.#
            mq_informer_->Get(AMQP_NOACK);
            AMQPMessage *m = mq_informer_->getMessage();
            if (m->getMessageCount() > -1)
            {
                Log::gdb("BaseCore::ProcessMQ informer: %s",m->getRoutingKey().c_str());
                if(m->getRoutingKey() == "informer.update")
                {
                    pdb->InformerUpdate(toString(m));
                }
                else if(m->getRoutingKey() == "informer.delete")
                {
                    pdb->InformerRemove(toString(m));
                }
                else if(m->getRoutingKey() == "informer.updateRating")
                {
                    pdb->loadRating(toString(m));
                }

                return true;
            }
        }
    }
    catch (AMQPException &ex)
    {
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

    //Загрузили все информеры
    pdb->InformerLoadAll();
    //Загрузили все кампании
    Campaign::loadAll(Config::Instance()->pDb->pDatabase);
    //Загрузили все предложения
    pdb->OfferLoad();
    //Загрузили все категории
    pdb->CategoriesLoad();
    //загрузили рейтинг
    pdb->loadRating();

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
        amqp_ = new AMQP(Config::Instance()->mq_path_);
        exchange_ = amqp_->createExchange();
        exchange_->Declare("getmyad", "topic", AMQP_AUTODELETE);
        //LogToAmqp("AMQP is up");

        // Составляем уникальные имена для очередей
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::string postfix = to_iso_string(now);
        boost::replace_first(postfix, ".", ",");

        std::string mq_advertise_name( "getmyad.advertise." + postfix );
        std::string mq_campaign_name( "getmyad.campaign." + postfix );
        std::string mq_informer_name( "getmyad.informer." + postfix );

        // Объявляем очереди
        mq_campaign_ = amqp_->createQueue();
        mq_campaign_->Declare(mq_campaign_name, AMQP_AUTODELETE | AMQP_EXCLUSIVE);
        mq_informer_ = amqp_->createQueue();
        mq_informer_->Declare(mq_informer_name, AMQP_AUTODELETE | AMQP_EXCLUSIVE);
        mq_advertise_ = amqp_->createQueue();
        mq_advertise_->Declare(mq_advertise_name, AMQP_AUTODELETE | AMQP_EXCLUSIVE);

       // Привязываем очереди
        exchange_->Bind(mq_advertise_name, "advertise.#");
        exchange_->Bind(mq_campaign_name, "campaign.#");
        exchange_->Bind(mq_informer_name, "informer.#");

       Log::info("Created ampq queues: %s, %s, %s",
                  mq_campaign_name.c_str(),
                  mq_informer_name.c_str(),
                  mq_advertise_name.c_str());
    }
    catch (AMQPException &ex)
    {
        Log::err("Error in AMPQ init: %s, Feature will be disabled.", ex.getMessage().c_str());
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
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    int millisecs_since_last_access =
        (now - last_time_accessed).total_milliseconds();
    int millisecs_since_start =
        (now - time_service_started_).total_milliseconds();
    int requests_per_second_current = 0;
    int requests_per_second_average = 0;

    if (millisecs_since_last_access)
        requests_per_second_current =
            (request_processed_ - last_time_request_processed) * 1000 /
            millisecs_since_last_access;
    if (millisecs_since_start)
        requests_per_second_average = request_processed_ * 1000 /
                                      millisecs_since_start;

    last_time_accessed = now;
    last_time_request_processed = request_processed_;

    out << "<html>";
    out << "<head>";
    out << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>";
    out << "<style type=\"text/css\">";
    out << "body { font-family: Arial, Helvetica, sans-serif; }";
    out << "h1, h2, h3 {font-family: \"georgia\", serif; font-weight: 400;}";
    out << "table { border-collapse: collapse; border: 1px solid gray; }";
    out << "td { border: 1px dotted gray; padding: 5px; font-size: 10pt; }";
    out << "th {border: 1px solid gray; padding: 8px; font-size: 10pt; }";
    out << "</style>";
    out << "</head>";
    out << "<body>";
    out << "<h1>Состояние службы Yottos GetMyAd worker</h1>";
    out << "<table>";
    out << "<tr>";
    out << "<td>Обработано запросов:</td> <td><b>" << request_processed_;
    out << "</b> (" << requests_per_second_current << "/сек, ";
    out << " в среднем " << requests_per_second_average << "/сек) ";
    out << "</td></tr>";
    out << "<tr>";
    out << "<td>Общее кол-во показов:</td> <td><b>" << offer_processed_;
    out << "</b> (" << social_processed_ << " из них социальная реклама) ";
    out << "</td></tr>";
    out << "<tr><td>Имя сервера: </td> <td>" <<
        (getenv("SERVER_NAME")? getenv("SERVER_NAME"): "неизвестно") <<
        "</td></tr>";
    out << "<tr><td>IP сервера: </td> <td>" <<Config::Instance()->server_ip_ <<"</td></tr>";
    out << "<tr><td>Текущее время: </td> <td>" <<
        boost::posix_time::second_clock::local_time() <<
        "</td></tr>";

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
        out << "</td></tr>";
    }
    catch (mongo::DB::NotRegistered &)
    {
        out << "Основная база не определена</td></tr>";
    }

    out << "<tr><td>База данных Redis (краткосрочная история):</td> <td>" <<
        Config::Instance()->redis_short_term_history_host_ << ":";
    out << Config::Instance()->redis_short_term_history_port_;
    out << "(TTL =" << Config::Instance()->shortterm_expire_ << ")<br/>";
    out << "status = " << (Config::Instance()->range_short_term_ > 0 ? "true" : "false");
    out << "</td></tr>";

    out << "<tr><td>База данных Redis (долгосрочная история):</td> <td>" <<
        Config::Instance()->redis_long_term_history_host_ << ":";
    out << Config::Instance()->redis_long_term_history_port_;
    out << "(TTL =0)<br/>";
    out << "status = " << (Config::Instance()->range_long_term_ > 0 ? "true" : "false");
    out << "</td></tr>";

    out << "<tr><td>База данных Redis (история показов):</td> <td>" <<
        Config::Instance()->redis_user_view_history_host_ << ":";
    out << Config::Instance()->redis_user_view_history_port_;
    out << "(TTL =" << Config::Instance()->views_expire_ << ")<br/>";
    out << "status = true";
    out << "</td></tr>";

    out << "<tr><td>База данных Redis (констекст страниц):</td> <td>" <<
        Config::Instance()->redis_long_term_history_host_ << ":";
    out << Config::Instance()->redis_long_term_history_port_;
    out << "(TTL =" << Config::Instance()->context_expire_ << ")<br/>";
    out << "status = " << (Config::Instance()->range_context_ > 0 ? "true" : "false");
    out << "</td></tr>";

    out << "<tr><td>База данных Redis (категорий):</td> <td>" <<
        Config::Instance()->redis_category_host_ << ":";
    out << Config::Instance()->redis_category_port_;
    out << "(TTL =0)<br/>";
    out << "status = false";
    out << "</td></tr>";

    out << "<tr><td>База данных Redis (ретаргетинг):</td> <td>" <<
        Config::Instance()->redis_retargeting_host_ << ":";
    out << Config::Instance()->redis_retargeting_port_;
    out << "(TTL =" << Config::Instance()->retargeting_by_time_ << ")<br/>";
    out << "status = true";
    out << "</td></tr>\n";

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
        out << "База данных журналирования не определена</td></tr>";
    }

    out << "<tr><td>Время запуска:</td> <td>" << time_service_started_ <<
        "</td></tr>" <<
        "<tr><td>AMQP:</td><td>" <<
        (amqp_? "активен" : "не активен") <<
        "</td></tr>";
    out <<  "<tr><td>Сборка: </td><td>" << __DATE__ << " " << __TIME__<<"</td></tr>";
    //out <<  "<tr><td>Драйвер mongo: </td><td>" << mongo::versionString << "</td></tr>";
    out << "</table>";
/*
    std::list<Campaign> campaigns = Campaign::all();
    out << "<p>Загружено <b>" << campaigns.size() << "</b> кампаний: </p>\n";
    out << "<table><tr>\n"
        "<th>Наименование</th>"
        "<th>Действительна</th>"
        "<th>Социальная</th>"
        "<th>Предложений</th>"
        "</tr>\n";

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
    out << "</table>";

    // Журнал сообщений AMQP
    out << "<p>Журнал AMQP: </p>"
        "<table>";
    int i = 0;
    for (auto it = mq_log_.begin(); it != mq_log_.end(); it++)
        out << "<tr><td>" << ++i << "</td>"
            "<td>" << *it << "</td></tr>";
    out << "<tr><td></td><td>Последняя проверка сообщений: </td><tr>"
        "</table>";
    out << "</body>";
    out << "</html>";

    return out.str();
}

bool BaseCore::cmdParser(const std::string &cmd, std::string &offerId, std::string &campaignId)
{
    boost::regex exp("Offer:(.*),Campaign:(.*)");
    boost::cmatch pMatch;

    if(boost::regex_match(cmd.c_str(), pMatch, exp))
    {
        offerId = pMatch[1];
        campaignId = pMatch[2];
        return true;
    }
    return false;
}
