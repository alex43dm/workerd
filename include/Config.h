#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <list>

#include <tinyxml.h>

#include "DataBase.h"

extern unsigned long request_processed_;
extern unsigned long last_time_request_processed;
extern unsigned long offer_processed_;
extern unsigned long social_processed_;
extern unsigned long retargeting_processed_;

class Config
{
public:
    std::vector<std::string> mongo_main_host_;
    std::string mongo_main_db_;
    std::string mongo_main_set_;
    bool mongo_main_slave_ok_;
    std::string mongo_main_login_;
    std::string mongo_main_passwd_;
    std::vector<std::string> mongo_log_host_;
    std::string mongo_log_db_;
    std::string mongo_log_set_;
    bool mongo_log_slave_ok_;
    std::string mongo_log_login_;
    std::string mongo_log_passwd_;
    std::string mongo_log_collection_;

    std::string redis_short_term_history_host_;
    std::string redis_short_term_history_port_;
    unsigned redis_short_term_history_timeout_;
    std::string redis_long_term_history_host_;
    std::string redis_long_term_history_port_;
    unsigned redis_long_term_history_timeout_;
    std::string redis_user_view_history_host_;
    std::string redis_user_view_history_port_;
    unsigned redis_user_view_history_timeout_;
    std::string redis_retargeting_host_;
    std::string redis_retargeting_port_;
    unsigned redis_retargeting_timeout_;
    std::string redis_category_host_;
    std::string redis_category_port_;
    unsigned redis_category_timeout_;
    int views_expire_;
    //int shortterm_expire_;

    float range_short_term_;
    float range_long_term_;
    float range_context_;
    float range_search_;
    float range_category_;

    //new params
    std::string server_ip_;
    std::string redirect_script_;
    std::string geocity_path_;
    std::string server_socket_path_;
    int server_children_;
    std::string dbpath_;
    std::string cookie_name_;
    std::string cookie_domain_;
    std::string cookie_path_;
    std::string db_dump_path_;
    std::string db_geo_csv_;

    std::string sphinx_host_;
    int         sphinx_port_;
    std::string sphinx_index_;
    const char **sphinx_field_names_;
    int         *sphinx_field_weights_;
    int         sphinx_field_len_;
    std::string shpinx_match_mode_;
    std::string shpinx_rank_mode_;
    std::string shpinx_sort_mode_;

    int         instanceId;
    std::string lock_file_;
    std::string pid_file_;
    std::string user_;
    std::string group_;

    int retargeting_percentage_;
    int retargeting_by_time_;
    unsigned offer_by_campaign_unique_;

    std::string template_teaser_;
    std::string template_banner_;
    std::string template_error_;
    std::string swfobject_;
    int time_update_;
    std::string mq_path_;
    std::string offerSqlStr, informerSqlStr, retargetingOfferSqlStr,campaingSqlStr;

    bool logCoreTime, logOutPutSize, logIP, logCountry, logRegion, logCookie,
        logContext, logSearch, logInformerId, logLocation,
        //retargeting offer ids from redis
        logRetargetingOfferIds,
        //output offer ids
        logOutPutOfferIds,
        logSphinx,
        logMonitor, logMQ, logRedis
        ;
    bool toLog()
    {
        return logCoreTime || logOutPutSize || logIP || logCountry || logRegion || logCookie
        || logContext || logSearch || logInformerId || logLocation || logRetargetingOfferIds
        || logOutPutOfferIds || logSphinx;
    }

    std::map<unsigned,std::string> Categories;

    DataBase *pDb;

    static Config* Instance();
    bool LoadConfig(const std::string fName);
    bool Load();
    void dbInit(){ pDb = new DataBase(true); }
    bool ReLoad();
    bool Save();
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
    std::string mes;
    std::string mFileName;
    std::string cfgFilePath;

    int getTime(const char *p);
    std::string getFileContents(const std::string &fileName);
    void redisHostAndPort(TiXmlElement *p, std::string &host, std::string &port, unsigned&);
    void exit(const std::string &mes);
    bool checkPath(const std::string &path_, bool checkWrite, bool isFile, std::string &mes);
};

extern Config *cfg;

#endif // CONFIG_H
