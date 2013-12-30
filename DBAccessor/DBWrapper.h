/**
 * Добавлено RealInvest Soft
 */
#ifndef DBWrapper_H
#define DBWrapper_H

#include "DBConnectionParams.h"
#include "UserHistory.h"
#include "Offer.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>


#include "Log.h"
#include "Core.h"

using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;

namespace redisDB
{
/** \brief

    Класс, обеспечивающий взаимодействие с базами данных Redis.

    Класс обеспечивает доступ к четырем базам данных: </BR>
	- база данных краткосрочной истории пользователя</BR>
	- база данных долгосрочной истории пользователя</BR>
	- база данных истории показов пользователя</BR>
	- база данных истории контекста страниц</BR>

	Все базы имеют одинаковую структуру. Ключом выступает индификатор пользователя (кукиес-ip).
	В качестве значения ключа используется SortedSet.

	Значение score вычисляется по следующей формуле:
	\code

		score = (second_clock::local_time() - d(1970,Jan,1)) / 1000000;

    \endcode

	База данных краткосрочной истории содержит к качестве значения ключа строку запроса, заданную
	пользователем в поисковике. Предполагается что для каждого ключа хранится не более двух
	предудыщих запросов.

	Например:
	\code
    // Добавление в иcторию пользователя с индификатором 1346593970-77.7.16.112 значения строки с поисковика
	// query="Nokia phone".
	// В качестве веса берется величина, вычисленная по дате последнего обновления значения
	ZADD 1346593970-77.7.16.112 123123123 "Nokia phone"
    \endcode

	База данных истории контекста страницы содержит к качестве значения ключа строку запроса, переданую
	модулю JavaScript загрузчиком. Предполагается что для каждого ключа хранится не более двух
	предудыщих запросов.

	Например:
	\code
    // Добавление в иcторию пользователя с индификатором 1346593970-77.7.16.112 значения строки контекста страницы
	// context="Linux iptables hfproxy".
	// В качестве веса берется величина, вычисленная по дате последнего обновления значения
	ZADD 1346593970-77.7.16.112 123123123 "Linux iptables hfproxy"
    \endcode

	База данных долгосрочной истории содержит к качестве значения ключа ключевые слова,
	соответствующие рекламному предложению, по которому пользователь выполнил переход.
	Модуль GR-M не отвечает за заполнение данной базы, а использует ее только для чтения.
	Предполагается, что база заполнена следуюшим образом:

	Пример:

	\code
    // Добавление в долгосрочную иcторию пользователя с индификатором 1346593970-77.7.16.112 информации о выборе
	// рекламного предложения имеющего ключевые слова "Nokia" и "дорогой телефон"

	ZADD 1346593970-77.7.16.112  3  "Nokia"					//если ключевое слово встречалось ранее, то его вес увеличивается на единицу
	ZADD 1346593970-77.7.16.112  1  "дорогой телефон"			//если ранее это слово не встречалось, то score=1
    \endcode

	База данных истории показов пользователя хранит информацию о количестве доступных показов
	рекламных предложений, которые имеют ограничения на показ.

	Например:
	\code
    // Добавление в иcторию пользователя с индификатором 1346593970-77.7.16.112 информации о том, что
	// рекламное предложение  "guid": "464fa8fd-1d3c-43e4-80e7-e2c38a911b6c" может быть
	// может быть показано пользователю еще 2 раза

	ZADD 1346593970-77.7.16.112  2  "464fa8fd-1d3c-43e4-80e7-e2c38a911b6c"					//доступное количество показов
    \endcode
 */
class DBWrapper
{
private:
    ///redis-client для работы с краткосрочной историей
//    boost::shared_ptr<redis::client> shortterm;

    ///redis-client для работы с долгосрочной историей
//    boost::shared_ptr<redis::client> longterm;

    ///redis-client для работы с историей показов
//    boost::shared_ptr<redis::client> views;

    ///redis-client для работы с контекстом страницы
//    boost::shared_ptr<redis::client> pagekeywords;

    ///redis-client для работы с категориями
//    boost::shared_ptr<redis::client> category;

    ///redis-client для работы с категориями
//    boost::shared_ptr<redis::client> retargeting;

    ///время жизни ключей в базе данных краткосрочной истории
    int shortterm_expire;

    ///время жизни ключей в базе данных долгосрочной истории
    int views_expire;

    ///время жизни ключей в базе данных контекстной истории
    int context_expire;

    DBConnectionParams params_;

    static const int VIEWS_LIMIT = 70;

public:


    DBWrapper()
    {

    }

    ~DBWrapper()
    {

    }


    /** \brief  Обновление истории показов пользователя

        \param ip        идентификатор пользователя
        \param result    вектор выбранных к показу рекламных предложений

    	Если выбранное к показу рекламное предложение имеет ограничение на количество показов конкретному пользователю,
    	то соответствующее рекламное предложение заносится в базу данных истории показов пользователя.

    	Если данное рекламное предложение показано пользователю впервые, то оно заносится в базу
    	с весом на единицу меньшим, чем возможное количество показов.
    	Например, если рекламное предложение "guid": "464fa8fd-1d3c-43e4-80e7-e2c38a911b6c" может быть показано конкретному
    	пользователю не более двух раз, то при первом просмотре данного рекламного предложения
    	в базу занесется следующее значение:

    	\code
    	ZADD 1346593970-77.7.16.112  1  "464fa8fd-1d3c-43e4-80e7-e2c38a911b6c"
    	\endcode

    	Если выбранное к показу рекламное предложение показывается пользователю не в первые,
    	то вес рекламного предложения уменьшается на единицу. Например, при повторном показе
    	рекламного предложения "guid": "464fa8fd-1d3c-43e4-80e7-e2c38a911b6c" история пользователя будет обновлена следующим образом:
    	\code
    	ZINCRBY 1346593970-77.7.16.112  -1  "464fa8fd-1d3c-43e4-80e7-e2c38a911b6c"
    	\endcode

    	Поскольку согласно алгоритму отбора рекламных предложений пользователю не могут быть
    	показаны предложения, исчерпавшие свой лимит показа, то в БД истории показов вес рекламных предложений не можут становится
    	отрицательным.

    	Рекламные предложение, не имеющие ограничения на количества показов, в базе данных истории показов пользователя
    	не регистрируются.

    	При создании ключа для конкретного пользователя ему выставляется время жизни, которое задается параметром views_expire.
     */
    void updateDeprecatedUserHistory(const std::string & ip, const vector<Offer> &result)
    {
        /*
        try
        {
            if (!getViewHistoryStatus())
                views = init_non_cluster_clientt(params_.redis_user_view_history_host_, params_.redis_user_view_history_port_);
                Log::info( "updateDeprecatedUserHistory: redis host: %s port: %d",
                          params_.redis_user_view_history_host_.c_str(), params_.redis_user_view_history_port_);
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis client does NOT reconnect! Probably redis-server for views history is failed!");
            return;
        }

        redis::client & db_views = *views;
        try
        {
            vector<Core::ImpressionItem>::const_iterator p = result.begin();
            while (p != result.end())
            {
                if (p->offer.uniqueHits!=-1)
                {
                    if (db_views.exists(ip))
                    {
                        long int k = -1;
                        try
                        {
                            k =  db_views.zrank(ip,p->offer.id);
                        }
                        catch (redis::redis_error & e)
                        {
                            //LOG(INFO) << "Redis exception: " << e.what() << endl;
                        }

                        if (k==-1)
                        {
                            try
                            {
                                db_views.zadd(ip,(p->offer.uniqueHits)-1,p->offer.id);
                            }
                            catch (redis::redis_error & e)
                            {
                                //LOG(INFO) << "Redis exception: " << e.what() << endl;
                            }
                        }
                        else
                        {
                            int temp = db_views.zscore(ip,p->offer.id);
                            if (temp>0)
                            {
                                try
                                {
                                    db_views.zincrby(ip, p->offer.id, -1);
                                }
                                catch (redis::redis_error & e)
                                {
                                    //LOG(INFO) << "Redis exception: " << e.what() << endl;
                                }
                            }
                        }
                    }
                    else
                    {
                        db_views.zadd(ip,(p->offer.uniqueHits)-1,p->offer.id);
                        db_views.expire(ip, views_expire);
                    }
                }
                p++;
            }
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis updateDeprecatedUserHistory failed!");
        }
        catch (...)
        {
            Log::err("Redis updateDeprecatedUserHistory failed with lexical cast error!");
        }
        */
    }



    /** \brief  Обновление краткосрочной истории пользователя

        \param ip        идентификатор пользователя
        \param query     строка запроса для добавления в историю

    	При переходе на сайт партнерской сети с поисковика отбор рекламных предложений производится в
    	соответсвии с этим запросом. После этого сам запрос заносится в базу данных краткосрочной истории пользователя.

    	Количество значений у ключей в данной базе не может привышать двух, таким образом в поиске по краткосрочной
    	истории участвует не более двух последних запросов.

    	Например:
    	\code
    	// Добавление в иcторию пользователя с ip = 192.168.1.1.04 значения строки с поисковика
    	// query="Nokia phone".
    	// В качестве веса берется величина, вычисленная по дате последнего обновления значения
    	ZADD 192.168.1.1.104 123123123 "Nokia phone"
    	\endcode

    	При создании ключа для конкретного пользователя ему выставляется время жизни, которое задается параметром \see shortterm_expire.
     */
    void updateShortHistory(const std::string & ip, const std::string & query)
    {
/*
        try
        {
            if (!getShortTermStatus())
                shortterm = init_non_cluster_clientt(params_.redis_short_term_history_host_, params_.redis_short_term_history_port_);
                Log::info( "updateShortHistory: redis host: %s port: %d",
                          params_.redis_short_term_history_host_.c_str(), params_.redis_short_term_history_port_);
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis client does NOT reconnect! Probably redis-server for short history is failed!");
            return;
        }

        try
        {
            if (!query.empty())
            {
                redis::client & db_shortterm = *shortterm;
                try
                {
                    db_shortterm.zadd(ip, currentDateToInt(), query);
                }
                catch (redis::redis_error & e)
                {
                    //LOG(INFO) << "not add only updated\n";
                }
                db_shortterm.expire(ip, shortterm_expire);
                if (db_shortterm.zcount(ip, 0,INFINITY)>=3)
                    db_shortterm.zremrangebyrank(ip, 0, 0);
            }
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis updateShortHistory failed!");
        }
        */
    }


    /** \brief  Обновление истории контекста страниц пользователя

        \param ip        идентификатор пользователя
        \param query     строка запроса для добавления в историю

    	При переходе на сайт партнерской сети а отбор рекламных предложений производится в
    	соответсвии с этим запросом. После этого сам запрос заносится в базу данных краткосрочной истории пользователя.

    	Количество значений у ключей в данной базе не может привышать двух, таким образом в поиске по
    	истории контекста участвует не более двух последних запросов.

    	Например:
        \code
        // Добавление в иcторию пользователя с индификатором 1346593970-77.7.16.112 значения строки контекста страницы
        // context="Linux iptables hfproxy".
        // В качестве веса берется величина, вычисленная по дате последнего обновления значения
        ZADD 1346593970-77.7.16.112 123123123 "Linux iptables hfproxy"
        \endcode

    	При создании ключа для конкретного пользователя ему выставляется время жизни, которое задается параметром \see context_expire.
     */
    void updateContextHistory(const std::string & ip, const std::string & query)
    {
/*
        try
        {
            if (!getPageKeywordsStatus())
                pagekeywords = init_non_cluster_clientt(params_.redis_page_keywords_host_, params_.redis_page_keywords_port_);
                Log::info( "updateContextHistory: redis host: %s port: %d",
                          params_.redis_page_keywords_host_.c_str(), params_.redis_page_keywords_port_);
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis client does NOT reconnect! Probably redis-server for short history is failed!");
            return;
        }

        try
        {
            if (!query.empty())
            {
                redis::client & db_pagekeywords = *pagekeywords;
                try
                {
                    db_pagekeywords.zadd(ip, currentDateToInt(), query);
                }
                catch (redis::redis_error & e)
                {
                    //LOG(INFO) << "not add only updated\n";
                }
                db_pagekeywords.expire(ip, context_expire);
                if (db_pagekeywords.zcount(ip, 0,INFINITY)>=3)
                    db_pagekeywords.zremrangebyrank(ip, 0, 0);
            }
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis updateContextHistory failed!");
        }
        */
    }

    /** \brief  Формирование объекта UserHistory по его идентификатору

        \param ip        идентификатор пользователя

    	Объект UserHistory представляет собой совокупность историй конкретного пользователя: краткосрочной истории,
    	долгосрочной истории и истории показов.

    	При формировании краткосрочной истории берутся все значения ключа из базы данных краткосрочной истории.

    	При формировании долгосрочной истории берутся ПЯТЬ наибольших по весу ключевых фраз с помощью конструкции

    	\code
    	ZREVRANGEBYSCORE ip +inf 0 LIMIT 5
    	\endcode

    	При формировании истории показов берутся те рекламные предложения, которые не могут быть показаны данному пользователю,
    	т.е. такие рекламные предложения, чей вес равен 0.

    	\code
    	ZRANGEBYSCORE ip -1 0
    	\endcode


    */
    UserHistory * getUserHistoryById(const std::string & ip)
    {

        UserHistory * userHistory = new UserHistory (ip);
        /*
        try
        {
            redis::client::string_vector history;
            redis::client & db_shortterm= *shortterm;
            if (db_shortterm.exists(ip))
            {
                db_shortterm.zrevrange(ip, 0, -1, history);
                for (unsigned int i=0; i<history.size(); i++)
                {
                    userHistory->addShortHistory(history[i]);
                }
            }
            redis::client::string_vector contextHistory;
            redis::client & db_pagekeywords= *pagekeywords;
            if (db_pagekeywords.exists(ip))
            {
                db_pagekeywords.zrevrange(ip, 0, -1, contextHistory);
                for (unsigned int i=0; i<contextHistory.size(); i++)
                {
                    userHistory->addContextHistory(contextHistory[i]);
                }
            }

            redis::client & db_longterm= *longterm;
            redis::client::string_vector slivki;
            if (db_longterm.exists(ip))
            {
                db_longterm.zrevrangebyscore(ip, INFINITY, 0.0, slivki, 0,10);
                for (unsigned int i=0; i<slivki.size(); i++)
                {
                    userHistory->addLongHistory(slivki[i]);
                }
            }
            redis::client & db_category= *category;
            redis::client::string_vector cat;
            if (db_category.exists(ip))
            {
                db_category.zrevrangebyscore(ip, INFINITY, 0.0, cat, 0,10);
                for (unsigned int i=0; i<cat.size(); i++)
                {
                    userHistory->addCategory(cat[i]);
                }
            }
            redis::client & db_retargeting= *retargeting;
            redis::client::string_vector ret;
            if (db_retargeting.exists(ip))
            {
                db_retargeting.zrevrangebyscore(ip, INFINITY, 0.0, ret, 0,10);
                for (unsigned int i=0; i<ret.size(); i++)
                {
                    userHistory->addRetargeting(ret[i]);
                }
            }
            redis::client & db_views= *views;
            history.clear();
            if (db_views.exists(ip))
            {
                db_views.zrangebyscore(ip, -1.0, 0.0, history);
                for (unsigned int i=0; i<history.size(); i++)
                {
                    userHistory->addDeprecatedOffer(history[i]);
                }
                if (history.size() > 800)
                    cutDeprecatedUserHistory(ip);
            }
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis getUserHistoryById  failed!");
            try
            {
                if (!getViewHistoryStatus()) views = init_non_cluster_clientt(params_.redis_user_view_history_host_, params_.redis_user_view_history_port_);
                if (!getShortTermStatus()) shortterm = init_non_cluster_clientt(params_.redis_short_term_history_host_, params_.redis_short_term_history_port_);
                if (!getLongTermStatus()) longterm = init_non_cluster_clientt(params_.redis_long_term_history_host_, params_.redis_long_term_history_port_);
                if (!getPageKeywordsStatus()) pagekeywords = init_non_cluster_clientt(params_.redis_page_keywords_host_, params_.redis_page_keywords_port_);
                if (!getCategoryStatus()) category = init_non_cluster_clientt(params_.redis_category_host_, params_.redis_category_port_);
                if (!getRetargetingStatus()) retargeting = init_non_cluster_clientt(params_.redis_retargeting_host_, params_.redis_retargeting_port_);
                Log::err( "Redis client reconnect !");
            }
            catch (redis::redis_error & e)
            {
                Log::err("Redis client does NOT reconnect! Probably redis-server is failed!");

            }


        }*/
        return userHistory;

    }

    /** \brief  Инициализация подключения к базам данных краткосрочной истории, долгосрочной истории и истории показов.

        \param param        параметры подключения
    */
    int InitConnection(DBConnectionParams& param)
    {
        params_ = param;
        /*
        try
        {
            shortterm = init_non_cluster_clientt(param.redis_short_term_history_host_, param.redis_short_term_history_port_);
            longterm = init_non_cluster_clientt(param.redis_long_term_history_host_, param.redis_long_term_history_port_);
            views = init_non_cluster_clientt(param.redis_user_view_history_host_, param.redis_user_view_history_port_);
            pagekeywords = init_non_cluster_clientt(param.redis_page_keywords_host_, param.redis_page_keywords_port_);
            category = init_non_cluster_clientt(param.redis_category_host_, param.redis_category_port_);
            retargeting = init_non_cluster_clientt(param.redis_retargeting_host_, param.redis_retargeting_port_);

            shortterm_expire = param.shortterm_expire_;
            views_expire = param.views_expire_;
            context_expire = param.context_expire_;

            Log::info( "redis connections inited");
        }
        catch (redis::redis_error & e)
        {
            Log::err("redis connection failed!");
            return -1;
        }
        */
        return 1;
    }

    /** \brief  Статус подключения к базе данных краткосрочной истории.

    	Возвращает 1 в случае, если подключение работает, и 0 в противном случае.
    */
    int getShortTermStatus()
    {/*
        try
        {
            redis::client & db_shortterm= *shortterm;
            db_shortterm.exists("test");
            return 1;
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis DB error. Short term DB is down.");
            return 0;
        }*/
        return 0;
    }

    /** \brief  Статус подключения к базе данных долгосрочной истории.

    	Возвращает 1 в случае, если подключение работает, и 0 в противном случае.
    */
    int getLongTermStatus()
    {/*
        try
        {
            redis::client & db_longterm= *longterm;
            db_longterm.exists("test");
            return 1;
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis DB error. Long term DB is down.");
            return 0;
        }*/
        return 0;
    }

    /** \brief  Статус подключения к базе данных истории показов пользователя

    	Возвращает 1 в случае, если подключение работает, и 0 в противном случае.
    */
    int getViewHistoryStatus()
    {/*
        try
        {
            redis::client & db_views= *views;
            db_views.exists("test");
            return 1;
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis DB error. View history DB is down.");
            return 0;
        }*/
        return 0;
    }




    /** \brief  Статус подключения к базе данных истории показов пользователя

    	Возвращает 1 в случае, если подключение работает, и 0 в противном случае.
    */
    int getPageKeywordsStatus()
    {/*
        try
        {
            redis::client & db_page= *pagekeywords;
            db_page.exists("test");
            return 1;
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis DB error. Page keywords DB is down.");
            return 0;
        }*/
    return 0;
    }

    /** \brief  Статус подключения к базе данных категорий

    	Возвращает 1 в случае, если подключение работает, и 0 в противном случае.
    */
    int getCategoryStatus()
    {/*
        try
        {
            redis::client & db_page= *category;
            db_page.exists("test");
            return 1;
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis DB error. Page keywords DB is down.");
            return 0;
        }*/
        return 0;
    }

    /** \brief  Статус подключения к базе данных категорий

    	Возвращает 1 в случае, если подключение работает, и 0 в противном случае.
    */
    int getRetargetingStatus()
    {/*
        try
        {
            redis::client & db_page= *retargeting;
            db_page.exists("test");
            return 1;
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis DB error. Page keywords DB is down.");
            return 0;
        }*/
        return 0;
    }
    /** \brief  Создание redis-client с параметрами по умолчанию

    Значения по умолчанию: localhost:6379

    boost::shared_ptr<redis::client> init_non_cluster_clientt()
    {
        const char* c_host = getenv("REDIS_HOST");
        string host = "localhost";
        if(c_host)
            host = c_host;
        cerr << "HOST" << "  " << host <<endl;

        return boost::shared_ptr<redis::client>( new redis::client(host) );
    }*/

    /** \brief  Создание redis-client

        \param server_host        	адрес сервера
        \param server_port     		порт сервера
    */
    /*
    boost::shared_ptr<redis::client> init_non_cluster_clientt(const std::string &server_host, int server_port)
    {
        return boost::shared_ptr<redis::client>( new redis::client(server_host,server_port) );
    }
*/



    /** \brief  Вычисление величины, привязанной к текущей дате.

    Значение score вычисляется по следующей формуле:
    \code

    	score = (second_clock::local_time() - d(1970,Jan,1)) / 1000000;

    \endcode

    Значение используется для выставления веса значений в базе данных краткосрочной истории и истории показов.
    */
    boost::int64_t currentDateToInt()
    {
        boost::gregorian::date d(1970,Jan,1);
        boost::posix_time::ptime myTime = microsec_clock::local_time();
        boost::posix_time::ptime myEpoch(d);
        boost::posix_time::time_duration myTimeFromEpoch = myTime - myEpoch;
        boost::int64_t myTimeAsInt = myTimeFromEpoch.ticks();
        return (myTimeAsInt%10000000000) ;
    }


    /* *
     *Очишяем историю показов пользователя
     * */
    void cutDeprecatedUserHistory(const std::string & ip)
    {/*
        redis::client & db_views = *views;
        try
        {
            db_views.del(ip);
        }
        catch (redis::redis_error & e)
        {
            Log::err("Redis cutDeprecatedUserHistory failed!");
        }*/
    }

};
#endif // DBWrapper_H

}  // end namespace redis
