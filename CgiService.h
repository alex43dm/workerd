#ifndef CGISERVICE_H
#define CGISERVICE_H

#include <string>
#include <fcgiapp.h>

#include "BaseCore.h"
#include "Config.h"

class Core;


/**
\brief Класс, оборачивающий проект как FastCgi (или просто CGI) сервис.


 Этот класс изолирует ядро сервиса от всей специфичной для веб-сервера
 информации. Он извлекает настройки сервера из командной строки, парсит
 строку запроса и передаёт эти "чистые" данные в экземляр класса \rel Core,
 который уже занимается реальной обработкой запроса.

	\see Serve().
 */
class CgiService
{

public:
     int socketId;
    /** Инициализирует сервис.
     *
     * Читает настройки из переменных окружения.
     *
     * Реально подключения к базам данных, RabbitMQ и другому происходит
     * при первом запросе в методе Serve().
     */
    CgiService(int argc, char *argv[]);
    ~CgiService();

    /** Начало обработки поступающих от FastCgi (или CGI) сервера запросов.
     *
     * Если запущен CGI сервером или просто из командной строки, то обработает
     * один запрос и вернёт управление.
     *
     * Если запущен FastCgi сервером, то будет ждать и обрабатывать следующие
     * запросы.
     */
    static void *Serve(void*);

private:
    /** Возвращает ответ FastCgi серверу.
     *
     * \param out       указатель на строку ответа.
     * \param status    HTTP статус ответа (например, 200, 404 или 500).
     * \param content_type Заголовок "Content-Type".
     * \param cookie    Заголовок "Set-Cookie". Параметр передается в виде обертки Cookie
     */
    void Response(FCGX_Request *stream, const std::string &out,
                  const std::string &c = "");

    /** Возвращает ответ FastCgi серверу.
     *
     * \param out       строка ответа.
     * \param status    HTTP статус ответа (например, 200, 404 или 500).
     * \param content_type Заголовок "Content-Type".
     * \param cookie    Заголовок "Set-Cookie". Параметр передается в виде обертки Cookie
     */
    void Response(FCGX_Request *stream, const std::string &out, int status,
                  const char *content_type = "text/html",
                  const std::string &cookie = "");


    /** Создаёт подключения к базам данных.
     *
     * Настройки читаются конструктором класса из переменных окружения среды.*/
    bool ConnectDatabase();

    /** Возвращает значение переменной окружения name или default_value,
     *  если значение не установлено.
     *
     *  \param name     переменная окружения.
     *  \param default_value значение по умолчанию.*/
    //std::string getenv(const char *name, const char *default_value);
/*
    char *getenv(const char *name)
    {
        return ::getenv(name);
    }
*/
    /** Обработка одного запроса.
     *
     * \param query     строка запроса (всё, что начинается после
     *                  вопросительного знака в URL запроса).
     * \param ip        IP посетителя, сделавшего запрос.
     * \param script_name название скрипта (то, что идёт до вопросительного
     *                  знака). Необходимо получать это значение от веб-сервера,
     *                  поскольку может меняться в зависимости от настроек
     *                  FastCGI. Используется для построения ссылки на новую
     *                  порцию предложений.
     */
    void ProcessRequest(FCGX_Request*, Core *);

private:

    int argc;
    char **argv;

    Config *cfg;
    BaseCore *bcore;

    /** Указатель на экземпляр ядра службы, который занимается реальной
     * обработкой запросов. */
 //   Core *core;

    /// \name Параметры подключения к основной базе данных.
    ///@{
    /** Адрес сервера баз данных. */
    std::string mongo_main_host_;

    /** Название базы данных. */
    std::string mongo_main_db_;

    /** Название группы реплик (если база данных находится в Replica Set). */
    std::string mongo_main_set_;

    /** Может ли сервис подключаться к slave серверам базам данных.
     *  Это балансирует нагрузку в режиме только для чтения, но появляется
     *  вероятность чтения не самых свежих данных (разница от мастера до
     *  нескольких секунд).
     */
    bool mongo_main_slave_ok_;
    ///@}


    /// \name Параметры подключения к базе данных журнала
    ///@{
    /** Адрес сервера баз данных. */
    std::string mongo_log_host_;

    /** Название базы данных. */
    std::string mongo_log_db_;

    /** Название группы реплик (если база данных находится в Replica Set). */
    std::string mongo_log_set_;

    /** Может ли сервис подключаться к slave серверам базам данных.
     *  Это балансирует нагрузку в режиме только для чтения, но появляется
     *  вероятность чтения не самых свежих данных (разница от мастера до
     *  нескольких секунд).
     */
    bool mongo_log_slave_ok_;
    ///@}

    /// \name Другие параметры сервера
    ///@{
    /** IP сервера. Ссылка на редирект по рекламному предложению содержит в
     *  себе адрес сервера, который её создал. */
    std::string server_ip_;

    /** Название скрипта обработки редиректа. */
    std::string redirect_script_;

    /** Путь к файлу базы данных MaxMind GeoIP City Edition. */
    std::string geoip_city_path_;
    ///@}

    /** IP-адрес  Redis-сервера с БД о краткосрочной истории пользователя */
    std::string redis_short_term_history_host_ ;
    /** Порт  Redis-сервера с БД о краткосрочной истории пользователя */
    std::string redis_short_term_history_port_ ;
    /** IP-адрес  Redis-сервера с БД о долгосрочной истории пользователя */
    std::string redis_long_term_history_host_ ;
    /** Порт  Redis-сервера с БД о долгосрочной истории пользователя */
    std::string redis_long_term_history_port_ ;
    /** IP-адрес  Redis-сервера с БД о истории показов пользователя */
    std::string redis_user_view_history_host_ ;
    /** Порт  Redis-сервера с БД о истории показов пользователя */
    std::string redis_user_view_history_port_ ;
    /** IP-адрес  Redis-сервера с БД о контексте страниц */
    std::string redis_page_keywords_host_ ;
    /** Порт  Redis-сервера с БД о контексте страниц */
    std::string redis_page_keywords_port_ ;
    /** IP-адрес  Redis-сервера с БД о категориях */
    std::string redis_category_host_ ;
    /** Порт  Redis-сервера с БД о категориях */
    std::string redis_category_port_ ;
    /** IP-адрес  Redis-сервера с БД о категориях */
    std::string redis_retargeting_host_ ;
    /** Порт  Redis-сервера с БД о категориях */
    std::string redis_retargeting_port_ ;

    /** Весовой коэффициент для РП, соответствующих ключевым словам*/
    float range_query_ ;
    /** Весовой коэффициент для РП, соответствующих кратскосрочной истории*/
    float range_short_term_ ;
    /** Весовой коэффициент для РП, соответствующих долгосрочной истории*/
    float range_long_term_ ;
    /** Весовой коэффициент для РП, соответствующих контексту страницы*/
    float range_context_ ;
    /** Весовой коэффициент для РП, соответствующих контексту страницы*/
    float range_context_term_ ;
    /** Весовой коэффициент для РП, соответствующих "рекламе на места"*/
    float range_on_places_ ;

    /** "Продолжительность жизни" ключей в базе краткосрочной истории */
    std::string shortterm_expire_;
    /** "Продолжительность жизни" ключей в базе истории контекста страницы */
    std::string context_expire_;
    /** "Продолжительность жизни" ключей в базе истории просмотров пользователя */
    std::string views_expire_;
    /** Название папки, в которой находится индекс по рекламным предложениям */
    std::string folder_offer_;
    /** Название папки, в которой находится индекс по информерам и рейтингам РП внутри этих информеров */
    std::string folder_informer_;

    pthread_t *threads;


    /** Инициализация модуля. */
    void RISinit();
    void RedisInit();
};

#endif
