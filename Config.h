#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <list>

#include <tinyxml.h>

class Config
{
public:
    std::string mongo_main_host_;
    std::string mongo_main_db_;
    std::string mongo_main_set_;
    std::string mongo_main_slave_ok_;
    std::string mongo_log_host_;
    std::string mongo_log_db_;
    std::string mongo_log_set_;
    std::string mongo_log_slave_ok_;
    std::string server_ip_;
    std::string redirect_script_;
    std::string geocity_path_;
    std::string redis_short_term_history_host_;
    std::string redis_short_term_history_port_;
    std::string redis_long_term_history_host_;
    std::string redis_long_term_history_port_;
    std::string redis_user_view_history_host_;
    std::string redis_user_view_history_port_;
    std::string redis_page_keywords_host_;
    std::string redis_page_keywords_port_;
    std::string redis_category_host_;
    std::string redis_category_port_;
    std::string redis_retargeting_host_;
    std::string redis_retargeting_port_;
    std::string range_query_;
    std::string range_short_term_;
    std::string range_long_term_;
    std::string range_context_;
    std::string range_context_term_;
    std::string range_on_places_;
    std::string shortterm_expire_;
    std::string views_expire_;
    std::string context_expire_;
    std::string folder_offer_;
    std::string folder_informer_;
    //new params
    std::string server_socket_path_;
    int server_children_;
    std::string dbpath_;
    std::string swfobject_js_;
    std::string geoGity_;
    std::string cookie_name_;
    std::string cookie_domain_;
    std::string cookie_path_;

    static Config* Instance();
    bool LoadConfig(const std::string fName);
    virtual ~Config();

    bool to_bool(std::string const& s)
    {
        return s != "false";
    }
    float to_float(std::string const& s)
    {
        return atof(s.c_str());
    }
    int to_int(std::string const& s)
    {
        return atoi(s.c_str());
    }

protected:
private:
    static Config* mInstance;
    Config();
    bool mIsInited;
    std::string mFileName;
    TiXmlDocument *mDoc;
    TiXmlElement* mRoot;
    TiXmlElement* mElem;
};

extern Config *cfg;

#endif // CONFIG_H
