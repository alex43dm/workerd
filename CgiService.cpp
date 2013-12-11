#include "DB.h"
#include <boost/algorithm/string.hpp>

#include "Log.h"
#include "CgiService.h"
#include "utils/UrlParser.h"
#include "utils/GeoIPTools.h"
#include "BaseCore.h"
#include "Core.h"
#include "Informer.h"
#include "utils/SearchEngines.h"
#include "HistoryManager.h"
#include "InformerTemplate.h"
#include <fcgi_stdio.h>
#include "utils/Cookie.h"
#include "DataBase.h"
#include "CHiredis.h"

#define THREAD_STACK_SIZE PTHREAD_STACK_MIN + 10 * 1024

using namespace std;
using namespace ClearSilver;
using namespace boost;

std::string time_t_to_string(time_t t);

string
convert (const posix_time::ptime& t)
{
    ostringstream ss;
    ss.exceptions(ios_base::failbit);

    date_time::time_facet<posix_time::ptime, char>* facet
        = new date_time::time_facet<posix_time::ptime, char>;
    ss.imbue(locale(locale::classic(), facet));

    facet->format("%a, %d-%b-%Y %T GMT");
    ss.str("");
    ss << t;
    return ss.str();
}

CgiService::CgiService(int argc, char *argv[])
    : argc(argc), argv(argv)
{
    std::string config = "/home/alex/Projects/worker/config.xml";

    cfg = Config::Instance();
    cfg->LoadConfig(config);

    mongo_main_host_ = cfg->mongo_main_host_;
    mongo_main_db_ = cfg->mongo_main_db_;
    mongo_main_set_ = cfg->mongo_main_set_;
    mongo_main_slave_ok_ = cfg->mongo_main_slave_ok_=="false" ? false : true;

    mongo_log_host_ = cfg->mongo_log_host_;
    mongo_log_db_ = cfg->mongo_log_db_;
    mongo_log_set_ = cfg->mongo_log_set_;
    mongo_log_slave_ok_ = cfg->mongo_log_slave_ok_ == "false" ? false : true;

    server_ip_ = cfg->server_ip_;

    redirect_script_ = cfg->redirect_script_;

    if (!GeoCity(cfg->geocity_path_.c_str()))
        Log::err("City database not found! City targeting will be disabled.");

    redis_short_term_history_host_ = cfg->redis_short_term_history_host_;
    redis_short_term_history_port_ = cfg->redis_short_term_history_port_;
    redis_long_term_history_host_ = cfg->redis_long_term_history_host_;
    redis_long_term_history_port_ = cfg->redis_long_term_history_port_;
    redis_user_view_history_host_ = cfg->redis_user_view_history_host_;
    redis_user_view_history_port_ = cfg->redis_user_view_history_port_;
    redis_page_keywords_host_ = cfg->redis_page_keywords_host_;
    redis_page_keywords_port_ = cfg->redis_page_keywords_port_;
    redis_category_host_ = cfg->redis_category_host_;
    redis_category_port_ = cfg->redis_category_port_;
    redis_retargeting_host_ = cfg->redis_retargeting_host_;
    redis_retargeting_port_ = cfg->redis_retargeting_port_;

    range_query_ = atof(cfg->range_query_.c_str());
    range_short_term_ = atof(cfg->range_short_term_.c_str());
    range_long_term_ = atof(cfg->range_long_term_.c_str());
    range_context_ = atof(cfg->range_context_.c_str());
    range_context_term_ = atof(cfg->range_context_term_.c_str());
    range_on_places_ = atof(cfg->range_on_places_.c_str());

    shortterm_expire_= cfg->shortterm_expire_;//24 hours default
    views_expire_ = cfg->views_expire_;//24 hours default
    context_expire_ = cfg->context_expire_;//24 hours default

    folder_offer_ = cfg->folder_offer_;
    folder_informer_ = cfg->folder_informer_;


    RISinit();

    bcore = new BaseCore();

    FCGX_Init();

    unlink(cfg->server_socket_path_.c_str());

    mode_t old_mode = umask(0);
    socketId = FCGX_OpenSocket(cfg->server_socket_path_.c_str(), cfg->server_children_);
    if(socketId < 0)
    {
        Log::err("Error open socket. exit");
        exit(1);
    }
    umask(old_mode);

    pthread_attr_t* attributes = (pthread_attr_t*) malloc(sizeof(pthread_attr_t));
    pthread_attr_init(attributes);
    pthread_attr_setstacksize(attributes, THREAD_STACK_SIZE);

    int i;
    threads = new pthread_t[cfg->server_children_ + 1];

    for(i = 0; i < cfg->server_children_; i++)
    {
        if(pthread_create(&threads[i], attributes, &this->Serve, this))
        {
            Log::err("creating thread failed");
        }
    }
    pthread_attr_destroy(attributes);
    //main loop
    for(;;)
    {
        //todo make lock on mq read
//        core->ProcessMQ();
        sleep(1);
    }

    //main loop end
    for(i = 0; i < cfg->server_children_; i++)
    {
        pthread_join(threads[i], 0);
    }
}

CgiService::~CgiService()
{
   delete []threads;
   delete bcore;
}

/*
string CgiService::getenv(const char *name, const char *default_value)
{
    char *value = getenv(name);
    return value? value : default_value;
}
*/

void CgiService::Response(FCGX_Request *req,
                          const std::string &out,
                          const std::string &cookie)
{
    FCGX_FPrintF(req->out,"Content-type: text/html\r\n");
    FCGX_FPrintF(req->out,"Set-Cookie: %s\r\n", cookie.c_str());
    FCGX_FPrintF(req->out,"Status: 200 OK\r\n");
    FCGX_FFlush(req->out);
    FCGX_FPrintF(req->out,"%s\r\n", out.c_str());

    FCGX_Finish_r(req);
}


void CgiService::Response(FCGX_Request *req, const std::string &out, int status,
                          const char *content_type, const string &cookie)
{
    Response(req, out.c_str(), status, content_type, cookie);
}

bool CgiService::ConnectDatabase()
{
    Log::info("Connecting to %s / %s", mongo_main_host_.c_str(), mongo_main_db_.c_str());

    try
    {
        if (mongo_main_set_.empty())
            mongo::DB::addDatabase(
                mongo_main_host_,
                mongo_main_db_,
                mongo_main_slave_ok_);
        else
            mongo::DB::addDatabase(
                mongo::DB::ReplicaSetConnection(
                    mongo_main_set_,
                    mongo_main_host_),
                mongo_main_db_,
                mongo_main_slave_ok_);

        if (mongo_log_set_.empty())
            mongo::DB::addDatabase( "log",
                                    mongo_log_host_,
                                    mongo_log_db_,
                                    mongo_log_slave_ok_);
        else
            mongo::DB::addDatabase( "log",
                                    mongo::DB::ReplicaSetConnection(
                                        mongo_log_set_,
                                        mongo_log_host_),
                                    mongo_log_db_,
                                    mongo_log_slave_ok_);

        // Проверяем доступность базы данных
        mongo::DB db;
        mongo::DB db_log("log");
        db.findOne("domain.categories", mongo::Query());

    }
    catch (mongo::UserException &ex)
    {
        Log::err("Error connecting to mongo: %s", ex.what());
        return false;
    }

    return true;
}


void *CgiService::Serve(void *data)
{
    CgiService *csrv = (CgiService*)data;

    Core *core = new Core(csrv->bcore->pDb);
    core->set_server_ip(csrv->server_ip_);
    core->set_redirect_script(csrv->redirect_script_);

    FCGX_Request request;

    if(FCGX_InitRequest(&request, csrv->socketId, 0) != 0)
    {
        Log::err("Can not init request");
        return nullptr;
    }
/*
    CHiredis *r = new CHiredis(csrv->cfg->redis_category_host_, csrv->cfg->redis_category_port_);
    r->connect();

    std::vector<std::string> rr;
    std::string key = "127.0.0.1-345sdg-343-ge46--344476rrh";
    r->getRange(key , 0, -1, rr);
    r->addVal(key, "test");
    r->addVal(key, "test1");
    r->addVal(key, "test2");
    r->getRange(key, 0, -1, rr);

    for(int i=0; i<rr.size();i++)
    {
        printf("%s\n",rr[i].c_str());
    }
*/
    for(;;)
    {
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&accept_mutex);
        int rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);

        if(rc < 0)
        {
            Log::err("Can not accept new request");
            break;
        }

        csrv->ProcessRequest(&request, core);
    }

    return nullptr;
}


void CgiService::ProcessRequest(FCGX_Request *req, Core *core)
{
    char *tmp_str = nullptr;
    std::string query, ip, script_name, visitor_cookie;


    if (!(tmp_str = FCGX_GetParam("QUERY_STRING", req->envp)))
    {
        Log::warn("query string is not set");
        return;
    }
    else
    {
        query = std::string(tmp_str);
        if(!query.size() || query == "/favicon.ico")
        {
            Response(req, "favicon.ico", "");
            return;
        }
    }

    tmp_str = nullptr;
    if( !(tmp_str = FCGX_GetParam("REMOTE_ADDR", req->envp)) )
    {
        Log::warn("remote address is not set");
        return;
    }
    else
    {
        ip = std::string(tmp_str);
    }

    tmp_str = nullptr;
    if (!(tmp_str = FCGX_GetParam("SCRIPT_NAME", req->envp)))
    {
        Log::warn("script name is not set");
        return;
    }
    else
    {
        script_name = std::string(tmp_str);
    }

    tmp_str = nullptr;
    if (!(tmp_str = FCGX_GetParam("HTTP_COOKIE", req->envp)))
    {
        //Log::warn("cookie is not set");
    }
    else
    {
        visitor_cookie = std::string(tmp_str);
    }

    UrlParser url(query);

    if (url.param("show") == "status")
    {
        //Response(req, core->Status(), 200);
        return;
    }

    using namespace boost::algorithm;
    std::vector<std::string> excluded_offers;
    std::string exclude = url.param("exclude");
    boost::split(excluded_offers, exclude, boost::is_any_of("_"));

    string cookie_name = cfg->cookie_name_;
    string cookie_value = time_t_to_string(time(NULL));


    std::vector<std::string> strs;
    boost::split(strs, visitor_cookie, boost::is_any_of(";"));

    for (unsigned int i=0; i<strs.size(); i++)
    {
        if(strs[i].find(cookie_name) != string::npos)
        {
            std::vector<std::string> name_value;
            boost::split(name_value, strs[i], boost::is_any_of("="));
            if (name_value.size() == 2)
                cookie_value = name_value[1];
        }
    }

    Cookie c = Cookie(cookie_name,
                      cookie_value,
                      Cookie::Credentials(Cookie::Authority(cfg->cookie_domain_),
                                          Cookie::Path(cfg->cookie_path_),
                                          Cookie::Expires(posix_time::second_clock::local_time() + years(1))));
    try
    {
        Params prm = Params()
                     .ip(ip)
                     .cookie_id(cookie_value)
                     .informer(url.param("scr"))
                     .country(url.param("country"))
                     .region(url.param("region"))
                     .test_mode(url.param("test") == "false")
                     .json(url.param("show") == "json")
                     .excluded_offers(excluded_offers)
                     .script_name(script_name.c_str())
                     .location(url.param("location"))
                     .w(url.param("w"))
                     .h(url.param("h"))
                     .search(url.param("search"))
                     .context(url.param("context"));

        vector<Core::ImpressionItem> items;
        string result = core->Process(prm, items);
        Response(req, result, c.to_string());
/*
        static pthread_mutex_t redis_mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&redis_mutex);
            core->ProcessSaveResults(prm, items);
        pthread_mutex_unlock(&redis_mutex);*/
    }
    catch (std::exception const &ex)
    {
        Response(req, "", 503);
        Log::err("exception %s: name: %s while processing: %s", typeid(ex).name(), ex.what(), query.c_str());
    }
}

/** Все необходимые действия для инициализации модуля вынесены в отдельный метод. */
void CgiService::RISinit()
{

    //подключаемся к mongo. операция критическая и без неё модуль не сможет работать.
    if (!ConnectDatabase())
    {
        // Возвращаем 503 на все запросы до тех пор, пока не подключимся
        while (!ConnectDatabase())
            sleep(1);
//            Response("Error connecting to database", 503);
//        Response("", 200);
    }

    /* инициализируем структуру соответствий поисковиков значениям параметров.
     * это необходимо для разбора строки запроса с поисковика.
     * если структура не проинициализирована, то модуль будет работать, но строка разбираться не будет.
     * операция не критическая (модуль не упадёт), но желательная, т.к. это функциональность нового модуля.*/
    //инициализация структуры, хранящей соответствия названий поисковиков зничениям параметров
    if(!SearchEngineMapContainer::instance()->setSearchEnginesMap("SearchEngines.txt"))
    {
        Log::warn("Хранилище соответствий поисковиков значениям параметров не проинициализировано. Строка запроса с поисковика, которую вбил пользователь, разбираться не будет.");
    }
    else
    {
        Log::info("Хранилище соответствий поисковиков значениям параметров проинициализировано.");
    }

    //инициализируем шаблоны. операция критическая, т.к. без неё модуль не сможет отображать баннеры в принципе.
    if (!InformerTemplate::instance()->init())
    {
        Log::err("Сбой при инициализации шаблонов.");
        while (InformerTemplate::instance()->init()==false)
        {
            sleep(1);
//            Response("Error templates' initialization", 503);
        }
//       Response("", 200);
    }
    else
    {
        Log::info("Шаблоны проинициализированы.");
    }
}

std::string time_t_to_string(time_t t)
{
    std::stringstream sstr;
    sstr << t;
    return sstr.str();
}
