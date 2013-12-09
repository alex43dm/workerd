#ifndef DBCONNECTIONPARAMS_H
#define DBCONNECTIONPARAMS_H

#include <string>



/**
\brief Класс, описывающий параметры подключения к базам данных Redis.

В классе собраны параметры, которые описывают настройки подключения к базам данных Redis.
*/
class DBConnectionParams
{
public:

    DBConnectionParams() {};
    DBConnectionParams(std::string redis_short_term_history_host,
                       std::string redis_short_term_history_port,
                       std::string redis_long_term_history_host,
                       std::string redis_long_term_history_port,
                       std::string redis_user_view_history_host,
                       std::string redis_user_view_history_port,
                       std::string redis_page_keywords_host,
                       std::string redis_page_keywords_port,
                       std::string redis_category_host,
                       std::string redis_category_port,
                       std::string redis_retargeting_host,
                       std::string redis_retargeting_port,
                       std::string shortterm_expire,
                       std::string context_expire,
                       std::string views_expire)
    {


        redis_short_term_history_host_ = redis_short_term_history_host;
        redis_short_term_history_port_ = atoi(redis_short_term_history_port.c_str());
        redis_long_term_history_host_ = redis_long_term_history_host;
        redis_long_term_history_port_ = atoi(redis_long_term_history_port.c_str());
        redis_user_view_history_host_ = redis_user_view_history_host;
        redis_user_view_history_port_ = atoi(redis_user_view_history_port.c_str());
        redis_page_keywords_host_ = redis_page_keywords_host;
        redis_page_keywords_port_ = atoi(redis_page_keywords_port.c_str());
        redis_category_host_ = redis_category_host;
        redis_category_port_ = atoi(redis_category_port.c_str());
        redis_retargeting_host_ = redis_retargeting_host;
        redis_retargeting_port_ = atoi(redis_retargeting_port.c_str());
        shortterm_expire_ = atoi(shortterm_expire.c_str());
        context_expire_ = atoi(context_expire.c_str());
        views_expire_ = atoi(views_expire.c_str());
    }

    /// Адрес сервера базы данных Redis краткосрочной истории
    std::string redis_short_term_history_host_ ;

    /// Порт сервера базы данных Redis краткосрочной истории
    int redis_short_term_history_port_ ;

    /// Адрес сервера базы данных Redis долгосрочной истории
    std::string redis_long_term_history_host_ ;

    /// Порт сервера базы данных Redis долгосрочной истории
    int redis_long_term_history_port_ ;

    /// Адрес сервера базы данных Redis истории показов пользователя
    std::string redis_user_view_history_host_ ;

    /// Порт сервера базы данных Redis истории показов пользователя
    int redis_user_view_history_port_ ;

    /// Адрес сервера базы данных Redis контекста страниц
    std::string redis_page_keywords_host_ ;

    /// Порт сервера базы данных Redis контекста страниц
    int redis_page_keywords_port_ ;

    /// Адрес сервера базы данных Redis категории
    std::string redis_category_host_ ;

    /// Порт сервера базы данных Redis категории
    int redis_category_port_ ;

    /// Адрес сервера базы данных Redis категории
    std::string redis_retargeting_host_ ;

    /// Порт сервера базы данных Redis категории
    int redis_retargeting_port_ ;

    /// Время жизни ключей в базе данных краткосрочной истории
    int shortterm_expire_;

    /// Время жизни ключей в базе данных истории показов пользователя
    int views_expire_;

    /// Время жизни ключей в базе данных истории показов пользователя
    int context_expire_;
};

#endif // PARAMS_H
