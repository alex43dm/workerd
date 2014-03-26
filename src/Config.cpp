#include <iostream>
#include <fstream>

#include <string.h>
#include <stdlib.h>

#include "Log.h"
#include "Config.h"
#include <assert.h>

unsigned long request_processed_;
unsigned long last_time_request_processed;
unsigned long offer_processed_;
unsigned long social_processed_;

bool is_file_exist(const std::string &fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

// Global static pointer used to ensure a single instance of the class.
Config* Config::mInstance = NULL;

Config* Config::Instance()
{
    if (!mInstance)   // Only allow one instance of class to be generated.
        mInstance = new Config();

    return mInstance;
}

Config::Config()
{
    mIsInited = false;
    mDoc = NULL;
}

bool Config::LoadConfig(const std::string fName)
{
    mFileName = fName;
    return Load();
}

void Config::redisHostAndPort(TiXmlElement *p, std::string &host, std::string &port)
{
    TiXmlElement *t1, *t2;

    if( (t1 = p->FirstChildElement("redis")) )
    {
        if( (t2 = t1->FirstChildElement("host")) && (t2->GetText()) )
        {
            host = t2->GetText();
        }

        if( (t2 = t1->FirstChildElement("port")) && (t2->GetText()) )
        {
            port = t2->GetText();
        }
    }
}

bool Config::Load()
{
    TiXmlElement *mel, *mels;

    if(!is_file_exist(mFileName))
    {
        std::cerr<<"does not found config file: "<<mFileName<<std::endl;
        ::exit(1);
    }

    Log::info("Config::Load: load file: %s",mFileName.c_str());

    mIsInited = false;
    mDoc = new TiXmlDocument(mFileName);

    if(!mDoc)
    {
        std::cerr<<"does not found config file: "<<mFileName<<std::endl;
        return mIsInited;
    }

    if(!mDoc->LoadFile())
    {
        std::cerr<<"error load file: "<<mFileName<<std::endl;
        ::exit(-1);
        return mIsInited;
    }

    mRoot = mDoc->FirstChildElement("root");

    if(!mRoot)
    {
        std::cerr<<"does not found root section in file: "<<mFileName<<std::endl;
        return mIsInited;
    }

    instanceId = atoi(mRoot->Attribute("id"));

    if( (mels = mRoot->FirstChildElement("mongo")) )
    {
        if( (mel = mels->FirstChildElement("main")) )
        {
            for(mElem = mel->FirstChildElement("host"); mElem; mElem = mElem->NextSiblingElement("host"))
            {
                mongo_main_host_.push_back(mElem->GetText());
            }

            if( (mElem = mel->FirstChildElement("db")) && (mElem->GetText()) )
                mongo_main_db_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("set")) && (mElem->GetText()) )
                mongo_main_set_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("slave")) && (mElem->GetText()) )
                mongo_main_slave_ok_ = strncmp(mElem->GetText(),"false", 5) > 0 ? false : true;

            if( (mElem = mel->FirstChildElement("login")) && (mElem->GetText()) )
                mongo_main_login_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("passwd")) && (mElem->GetText()) )
                mongo_main_passwd_ = mElem->GetText();
        }

        if( (mel = mels->FirstChildElement("log")) )
        {
            for(mElem = mel->FirstChildElement("host"); mElem; mElem = mElem->NextSiblingElement("host"))
            {
                mongo_log_host_.push_back(mElem->GetText());
            }

            if( (mElem = mel->FirstChildElement("db")) && (mElem->GetText()) )
                mongo_log_db_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("set")) && (mElem->GetText()) )
                mongo_log_set_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("slave")) && (mElem->GetText()) )
                mongo_log_slave_ok_ = strncmp(mElem->GetText(),"false", 5) > 0 ? false : true;

            if( (mElem = mel->FirstChildElement("login")) && (mElem->GetText()) )
                mongo_log_login_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("passwd")) && (mElem->GetText()) )
                mongo_log_passwd_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("collection")) && (mElem->GetText()) )
                mongo_log_collection_ = mElem->GetText();
        }

    }


    //main config
    if( (mElem = mRoot->FirstChildElement("server")) )
    {
        if( (mel = mElem->FirstChildElement("server_ip")) && (mel->GetText()) )
        {
            server_ip_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("redirect_script")) && (mel->GetText()) )
        {
            redirect_script_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("geocity_path")) && (mel->GetText()) )
        {
            geocity_path_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("socket_path")) && (mel->GetText()) )
        {
            server_socket_path_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("children")) && (mel->GetText()) )
        {
            server_children_ = atoi(mel->GetText());
        }

        if( (mel = mElem->FirstChildElement("dbpath")) && (mel->GetText()) )
        {
            dbpath_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("db_dump_path")) && (mel->GetText()) )
        {
            db_dump_path_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("db_geo_csv")) && (mel->GetText()) )
        {
            db_geo_csv_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("lock_file")) && (mel->GetText()) )
        {
            lock_file_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("pid_file")) && (mel->GetText()) )
        {
            pid_file_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("mq_path")) && (mel->GetText()) )
        {
            mq_path_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("user")) && (mel->GetText()) )
        {
            user_ = mel->GetText();
        }
        else
        {
            Log::warn("element dbpath is not inited: :memory:");
            dbpath_ = ":memory:";
        }

        if( (mel = mElem->FirstChildElement("time_update")) && (mel->GetText()) )
        {
            time_update_ = getTime(mel->GetText());
        }


        if( (mel = mElem->FirstChildElement("template_teaser")) && (mel->GetText()) )
            template_teaser_ = getFileContents(mel->GetText());
        else
        {
            Log::warn("element template_teaser is not inited");
        }

        if( (mel = mElem->FirstChildElement("template_banner")) && (mel->GetText()) )
            template_banner_ = getFileContents(mel->GetText());
        else
        {
            Log::warn("element template_banner is not inited");
        }

        if( (mel = mElem->FirstChildElement("template_error")) && (mel->GetText()) )
            template_error_ = getFileContents(mel->GetText());
        else
        {
            Log::warn("element template_error is not inited");
        }


        if( (mel = mElem->FirstChildElement("swfobject")) && (mel->GetText()) )
            swfobject_ = getFileContents(mel->GetText());
        else
        {
            Log::warn("element swfobject is not inited");
        }

        if( (mel = mElem->FirstChildElement("geoGity")) && (mel->GetText()) )
            geoGity_ = mel->GetText();
        else
        {
            Log::warn("element geoGity is not inited");
        }

        if( (mel = mElem->FirstChildElement("cookie_name")) && (mel->GetText()) )
            cookie_name_ = mel->GetText();
        else
        {
            Log::warn("element cookie_name is not inited");
        }
        if( (mel = mElem->FirstChildElement("cookie_domain")) && (mel->GetText()) )
            cookie_domain_ = mel->GetText();
        else
        {
            Log::warn("element cookie_domain is not inited");
        }

        if( (mel = mElem->FirstChildElement("cookie_path")) && (mel->GetText()) )
            cookie_path_ = mel->GetText();
        else
        {
            Log::warn("element cookie_path is not inited");
        }
    }

    //retargeting config
    if( (mElem = mRoot->FirstChildElement("retargeting")) )
    {
        if( (mel = mElem->FirstChildElement("persents")) && (mel->GetText()) )
        {
            retargeting_by_persents_ = strtol(mel->GetText(),NULL,10);
        }

        if( (mel = mElem->FirstChildElement("ttl")) && (mel->GetText()) )
        {
            retargeting_by_time_ = getTime(mel->GetText());
        }
        else
        {
            retargeting_by_time_ = 24*3600;
        }

        if( (mel = mElem->FirstChildElement("unique_by_campaign")) && (mel->GetText()) )
        {
            retargeting_unique_by_campaign_ = strncmp(mel->GetText(),"false", 5) > 0 ? false : true;
        }
        else
        {
            retargeting_unique_by_campaign_ = false;
        }

        redisHostAndPort(mElem, redis_retargeting_host_, redis_retargeting_port_);
    }

    //history
    TiXmlElement *history, *section;
    if( (history = mRoot->FirstChildElement("history")) )
    {
        //views
        if( (section = history->FirstChildElement("views")) )
        {
            if( (mel = section->FirstChildElement("ttl")) && (mel->GetText()) )
            {
                views_expire_ = getTime(mel->GetText());
            }
            else
            {
                views_expire_ = 24*3600;
            }

            redisHostAndPort(section, redis_user_view_history_host_, redis_user_view_history_port_);
        }
        //short term
        if( (section = history->FirstChildElement("short_term")) )
        {
            /*
            if( (mel = section->FirstChildElement("ttl")) && (mel->GetText()) )
            {
                shortterm_expire_ = getTime(mel->GetText());
            }
            else
            {
                shortterm_expire_ = 24*3600;
            }
*/
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                range_short_term_ = ::atof(mel->GetText());
            }

            redisHostAndPort(section, redis_short_term_history_host_, redis_short_term_history_port_);
        }
        //long term
        if( (section = history->FirstChildElement("long_term")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                range_long_term_ = ::atof(mel->GetText());
            }

            redisHostAndPort(section, redis_long_term_history_host_, redis_long_term_history_port_);
        }
    }


    if( (mElem = mRoot->FirstChildElement("sphinx")) )
    {
        if( (mel = mElem->FirstChildElement("host")) && (mel->GetText()) )
        {
            sphinx_host_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("port")) && (mel->GetText()) )
        {
            sphinx_port_ = atoi(mel->GetText());
        }

        if( (mel = mElem->FirstChildElement("index")) && (mel->GetText()) )
        {
            sphinx_index_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("mach_mode")) && (mel->GetText()) )
        {
            shpinx_match_mode_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("rank_mode")) && (mel->GetText()) )
        {
            shpinx_rank_mode_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("sort_mode")) && (mel->GetText()) )
        {
            shpinx_sort_mode_ = mel->GetText();
        }

        if((mel = mElem->FirstChildElement("fields")))
        {
            sphinx_field_names_ = (const char**)malloc(sizeof(const char**));
            sphinx_field_weights_ = (int *)malloc(sizeof(int*));
            sphinx_field_len_ = 0;
            for(mElem = mel->FirstChildElement(); mElem; mElem = mElem->NextSiblingElement(),sphinx_field_len_++)
            {
                sphinx_field_names_[sphinx_field_len_] = mElem->Value();
                sphinx_field_weights_[sphinx_field_len_] = atoi(mElem->GetText());
            }
        }

        Log::info("sphinx mach: %s, rank: %s sort: %s", shpinx_match_mode_.c_str(), shpinx_rank_mode_.c_str(), shpinx_sort_mode_.c_str());
    }

    pDb = new DataBase(true);

    request_processed_ = 0;
    last_time_request_processed = 0;
    offer_processed_ = 0;
    social_processed_ = 0;

    mIsInited = true;
    return mIsInited;
}

//---------------------------------------------------------------------------------------------------------------
Config::~Config()
{
    if (mDoc != NULL)
    {
        delete mDoc;
        mDoc = NULL;
    }

    delete pDb;

    mInstance = NULL;
}
//---------------------------------------------------------------------------------------------------------------
int Config::getTime(const char *p)
{
    struct tm t;
    int ret;
    strptime(p, "%H:%M:%S", &t);
    ret = t.tm_hour * 3600;
    ret = ret + t.tm_min * 60;
    return ret + t.tm_sec;
}
//---------------------------------------------------------------------------------------------------------------
std::string Config::getFileContents(const std::string &fileName)
{
    std::ifstream in(fileName, std::ios::in | std::ios::binary);

    if(in)
    {
        std::string cnt;
        in.seekg(0, std::ios::end);
        cnt.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&cnt[0], cnt.size());
        in.close();
        return(cnt);
    }

    Log::err("error open file: %s: %d",fileName.c_str(), errno);
    return std::string();
}
