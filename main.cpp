#include "CgiService.h"
#include "Log.h"

/** \mainpage GetMyAd worker

    \section usage_section Использование

    \b getmyad-worker --- это модуль, ответственный за непосредственное отображение
    рекламы на сайтах-партнёрах. Может подключаться тремя вариантами:

    -# Основной режим использования --- через FastCGI.
    -# Через CGI --- удобно для проверки работоспобности с сервером.
    -# Для удобства отладки может вызываться из командной строки с параметром
        QUERY_STRING, то есть той частью, которая находится после вопросительного
        знака в url.

    Примеры использования из командной строки:

    \code
	getmyad-worker ?scr=49A37671-D505-4B07-8BCA-0B3F65A99B73&country=UA
	getmyad-worker ?show=status
    \endcode


    \section settings_section Настройка сервиса

    getmyad-worker использует две базы данных mongo:

    -# Главная БД, которая содержит описание всех сущностей (информеры,
       кампании, предложения и т.д.). Это основная база данных проекта,
       которую использует пользовательская часть. На уровне приложения
       регистрируется как база данных по умолчанию (см. DB::addDatabase).
    -# БД для ведения журнала показов и кликов. Этих баз данных может быть
       несколько. С некоторой периодичностью данные из этих баз обрабатываются,
       и собранная статистика записываются в основную БД. Регистриуется под
       именем "log".

	 и четыре базы данных redis:

	 -# БД, содержащая краткосрочную историю пользователей.
	    Для каждого пользователя хранятся последние два его запроса с поисковика,
		по результатам обработки которых поисковой машиной он перешёл на сайт партнёрской сети.
	 -# БД, содержащая долгосрочную историю пользователей.
	    Для каждого пользователя ведётся и долгосрочная история.
	 -# БД, содержащая историю показов пользователей.
	    Для каждого пользователя ведётся история показов, обеспечивающая возможность показа конкретного
		рекламного предложения конкретному пользователю не более заданного количества раз.
	 -# БД, содержащая контекст страницы.
	    В данной версии модуля работа с базой данных контекста страницы не ведется,
		однако задавать параметры для этой базы всё равно нужно.


    Можно совместить обе БД mongo в одной, однако это не рекомендуется. Вариант,
    в котором на каждом сервере, выполняющем getmyad-worker, расположена своя
    БД для журналирования, является оптимальным с точки зрения архитектуры и
    производительности.

	Все четыре БД Redis совмещать нельзя. Реализованный вариант является
	оптимальным с точки зрения архитектуры и производительности.

    Настройка параметров подключения к базам данных mongo осуществляется
    передачей переменных окружения \c mongo_main_host, \c mongo_main_db,
    \c mongo_log_host и \c mongo_log_db, которые содержат адрес (в формате
    "host[:port]" и названия баз данных. Для подключения к кластеру баз данных
    (replica set) необходимо также указать непустой параметр \c mongo_main_set
    и/или \c mongo_log_set. В этом случае адрес базы данных может быть в
    формате "host1[:port1],host2[:port2],...".

    \c mongo_main_slave_ok и \c mongo_log_slave_ok могут быть равны true или
    false и определяют возможность read-only подключения к slave-серверам в
    кластере.

    По умолчанию будут использованы следующие параметры:

    \code
	mongo_main_host = localhost
	mongo_main_db = getmyad_db
	mongo_main_set = ''
	mongo_main_slave_ok = false
	mongo_log_host = localhost
	mongo_log_db = getmyad
	mongo_log_set = ''
	mongo_log_slave_ok = false
    \endcode

    Настройка параметров подключения к базам данных Redis осуществляется
	передачей переменных окружения (хосты и порты соответстующих баз): \n
	\c redis_short_term_history_host - Адрес сервера базы данных Redis краткосрочной истории,\n
	\c redis_short_term_history_port - Порт сервера базы данных Redis краткосрочной истории, \n
	\c redis_short_long_history_host - Адрес сервера базы данных Redis долгосрочной истории, \n
	\c redis_short_long_history_port - Порт сервера базы данных Redis долгосрочной истории, \n
	\c redis_user_view_history_host - Адрес сервера базы данных Redis истории показов пользователя, \n
	\c redis_user_view_history_port - Порт сервера базы данных Redis истории показов пользователя, \n
	\c redis_page_keywords_host - Адрес сервера базы данных Redis контекста страниц, \n
	\c redis_page_keywords_port - Порт сервера базы данных Redis контекста страниц.

	По умолчанию будут использованы следующие параметры:
	\code
	redis_short_term_history_host=127.0.0.1
	redis_short_term_history_port=6379
	redis_short_long_history_host=127.0.0.1
	redis_short_long_history_port=6380
	redis_user_view_history_host=127.0.0.1
	redis_user_view_history_port=6381
	redis_page_keywords_host=127.0.0.1
	redis_page_keywords_port=6382
	\endcode

	Для использования баз данных Redis необходимо настроить продолжительность "жизни" ключей в БД краткосройчной истории и
	БД истории показов. Настройка осуществляется передачей переменных окружения:\n
	\c shortterm_expire - Время жизни ключей в базе данных краткосрочной истории, \n
	\c views_expire - Время жизни ключей в базе данных истории показов пользователя.

	По умолчанию будут использованы следующие параметры:
	\code
	shortterm_expire=864000
	views_expire=864000
	\endcode

    Сервис должен знать свой адрес. Адрес должен задаваться в переменной
    окружения \c SERVER_ADDR.

    Адрес скрипта перенаправления на рекламное предложение (то, что засчитывает
    клик и отправляет клиента по ссылке рекламного предложения), задаётся в
    переменной \c REDIRECT_SCRIPT_URL. По умолчанию принимается "/redirect".

    Для реализации таргетинга по городам используется база данных MaxMind GeoIP.
    Путь к ней указывается в переменной окружения \c GEOIP_CITY_PATH. По умолчанию
    база ищется в файле /usr/share/GeoIP/GeoLiteCity.dat.

	Для настройки запроса к индексу необходимо задать веса запросов, в зависимости
	от значимости самого запроса. Задание весовых коэффициентов осуществляется передачей
	переменных окружения:
	\c range_query - весовой коэффициент для запроса к индексу по строке запроса пользователя, если он перешёл из поисковика, \n
	\c range_short_term - весовой коэффициент для запроса к индексу по краткосрочной истории пользователя, \n
	\c range_long_term - весовой коэффициент для запроса к индексу по долгосрочной истории пользователя, \n
	\c range_context - весовой коэффициент для запроса к индексу по контексту страницы.

	По умолчанию будут использованы следующие параметры:
	\code
	range_query=1
	range_short_term=0.75
	range_long_term=0.5
	range_context=0.25
	\endcode

	Модуль будет искать индекс в папке "/var/www/index".

	Для реализации возможности выделения строки запроса пользователя в одной папке с модулем должен находиться
	файл "SearchEngines.txt", в котором должны быть заданы поисковики и имена параметров, отвечающих за запрос
	пользователя.\n
	Пример записи данных в файле:
	\code
	google.: q=, as_q=, as=
	rambler.ru: query=, words=
	go.mail.ru/search_images: q=
	go.mail.ru: q=
	images.google.: q=
	search.live.com: q=
	rapidshare-search-engine: s=
	search.yahoo.com: p=
	nigma.ru/index.php: s=, q=
	search.msn.com/results: q=
	ask.com/web: q=
	search.qip.ru/search: query=
	rapidall.com/search.php: query=
	images.yandex.ru/: text=
	m.yandex.ru/search: query=
	hghltd.yandex.net: text=
	yandex.ru: text=
	\endcode

	Для реализации возможности отображения flash-баннеров в одной папке с модулем должен находиться файл "swfobject.js".
	Это файл с javascript-библиотекой swfobject. При написании модуля использовалась версия swfobject 2.2.




    \section query_params_section Параметры запроса

    Сервис использует следующие get-параметры, передаваемые в URL:

    \param scr      id информера, на котором осуществляется показ рекламы.
		            В основном (рабочем) режиме, это единственный обязательный
		            параметр.

    \param country	двухбувенный код страны, который перекрывает определение
	            	страны по ip. Позволяет просмотреть, как будет отображаться
		            информер в разных странах.

    \param region   название области в написании, принятом в базе MaxMind GeoCity.
                    Позволяет посмотреть, как будет отображаться информер в
            		различных областях. Следует учитывать, что параллельно с
            		таргетингом по областям действует таргетинг по странам,
            		поэтому для корректного отображения следует также передавать
            		параметр country.

    \param show=status  вывод информации о состоянии сервиса и минимальная
                    статистика.

    \param show=json    вывод предложений для информера в формате json.
                    Используется, например, при динамической подгрузке
                    предложений в информер.

    \param test     если равен true, то информация о показах не будет записана.
		            Это можно использовать, например, для служебных проверок
		            информера.

    \param exclude  список id рекламных предложений, которые нужно исключить из
		            результата. Id разделяются знаком '_' (нижнее подчёркивание).
            		Функция используется для исключения повторяющихся
                    предложений в листании страниц информера.

    \param location адрес страницы, на которой расположен информер. Обычно
            		передаётся из javascript загрузчика.

    \param referrer адрес страницы, с которой посетитель попал на сайт-партнёр.
            		Обычно передаётся из javascript загрузчика.

*/

Config *cfg;

#include "utils/GeoIPTools.h"

int main(int argc, char *argv[])
{
    Log(LOG_LOCAL0);

    CgiService(argc, argv).run();
    return 0;
}
