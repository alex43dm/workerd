#include <boost/algorithm/string.hpp>

#include "DB.h"
#include "Log.h"
#include "CgiService.h"
#include "UrlParser.h"
#include "GeoIPTools.h"
#include "BaseCore.h"
#include "Core.h"
#include "Informer.h"
#include "Cookie.h"
#include "DataBase.h"
#include "Server.h"

#include <fcgi_stdio.h>

#define THREAD_STACK_SIZE PTHREAD_STACK_MIN + 10 * 1024

std::string time_t_to_string(time_t t)
{
    std::stringstream sstr;
    sstr << t;
    return sstr.str();
}

CgiService::CgiService()
{

    if (!GeoCity(cfg->geocity_path_.c_str()))
        Log::err("City database not found! City targeting will be disabled.");

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
}

void CgiService::run()
{
   //main loop
    for(int i=0;;i++)
    {
        //todo make lock on mq read
        bcore->ProcessMQ();
        sleep(1);
        if( i >= Config::Instance()->time_update_ )
        {
            bcore->ReloadAllEntities();
            i = 0;
        }
    }
}

CgiService::~CgiService()
{
   for(int i = 0; i < cfg->server_children_; i++)
    {
        pthread_join(threads[i], 0);
    }

   delete []threads;
   delete bcore;
}

void CgiService::Response(FCGX_Request *req,
                          const std::string &out,
                          const std::string &cookie)
{
    FCGX_FPrintF(req->out,"Content-type: text/html\r\n");
    FCGX_FPrintF(req->out,"Set-Cookie: %s\r\n", cookie.c_str());
    FCGX_FPrintF(req->out,"Status: 200 OK\r\n");
    FCGX_FFlush(req->out);
    FCGX_FPrintF(req->out,"\r\n%s\n", out.c_str());
    FCGX_Finish_r(req);
}


void CgiService::Response(FCGX_Request *req, const std::string &out, int status,
                          const char *content_type, const string &cookie)
{
    Response(req, out.c_str(), status, content_type, cookie);
}


void *CgiService::Serve(void *data)
{
    CgiService *csrv = (CgiService*)data;

    Core *core = new Core();
//    core->set_server_ip(Config::Instance()->server_ip_);
//    core->set_redirect_script(Config::Instance()->redirect_script_);

    FCGX_Request request;

    if(FCGX_InitRequest(&request, csrv->socketId, 0) != 0)
    {
        Log::err("Can not init request");
        return nullptr;
    }

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
        Log::warn("cookie is not set");
    }
    else
    {
        visitor_cookie = std::string(tmp_str);
    }

    UrlParser url(query);

    if (url.param("show") == "status")
    {
        Response(req, bcore->Status(), "");
        return;
    }

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

    ClearSilver::Cookie c = ClearSilver::Cookie(cookie_name,
                      cookie_value,
                      ClearSilver::Cookie::Credentials(
                                ClearSilver::Cookie::Authority(cfg->cookie_domain_),
                                ClearSilver::Cookie::Path(cfg->cookie_path_),
                            ClearSilver::Cookie::Expires(boost::posix_time::second_clock::local_time() + boost::gregorian::years(1))));
    try
    {
        Params prm = Params()
                     .ip(ip)
                     .cookie_id(cookie_value)
                     .informer_id(url.param("scr"))
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

        std::string result;

        result = core->Process(&prm);
        Response(req, result, c.to_string());
        core->ProcessSaveResults();
    }
    catch (std::exception const &ex)
    {
        Response(req, "", 503);
        Log::err("exception %s: name: %s while processing: %s", typeid(ex).name(), ex.what(), query.c_str());
    }
}
