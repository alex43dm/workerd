#include <iostream>
#include <fstream>

#include <string.h>
#include <stdlib.h>

#include "Log.h"
#include "Config.h"
#include <assert.h>

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
    TiXmlElement *mel, *mels;

    mIsInited = false;
    mFileName = fName;
    mDoc = new TiXmlDocument(mFileName);

    if(!mDoc)
    {
        std::cerr<<"does not found config file: "<<fName<<std::endl;
        return mIsInited;
    }

    if(!mDoc->LoadFile())
    {
        std::cerr<<"error load file: "<<fName<<std::endl;
        return mIsInited;
    }

    mRoot = mDoc->FirstChildElement("root");

    if(!mRoot)
    {
        std::cerr<<"does not found root section in file: "<<fName<<std::endl;
        return mIsInited;
    }

    instanceId = atoi(mRoot->Attribute("id"));

    //main config
    if( (mElem = mRoot->FirstChildElement("server_ip")) && (mElem->GetText()) )
    {
        server_ip_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("redirect_script")) && (mElem->GetText()) )
    {
        redirect_script_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("geocity_path")) && (mElem->GetText()) )
    {
        geocity_path_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("shortterm_expire")) && (mElem->GetText()) )
    {
        shortterm_expire_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("views_expire")) && (mElem->GetText()) )
    {
        views_expire_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("context_expire")) && (mElem->GetText()) )
    {
        context_expire_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("folder_offer")) && (mElem->GetText()) )
    {
        folder_offer_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("folder_informer")) && (mElem->GetText()) )
    {
        folder_informer_ = mElem->GetText();
    }

    if( (mElem = mRoot->FirstChildElement("retargeting_by_persents")) && (mElem->GetText()) )
    {
        retargeting_by_persents_ = strtol(mElem->GetText(),NULL,10);
    }

    if( (mElem = mRoot->FirstChildElement("retargeting_by_time")) && (mElem->GetText()) )
    {
        retargeting_by_time_ = getTime(mElem->GetText());
    }
    else
    {
        retargeting_by_time_ = 24*3600;
    }

    if( (mels = mRoot->FirstChildElement("mongo")) )
    {
        if( (mel = mels->FirstChildElement("main")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                mongo_main_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("db")) && (mElem->GetText()) )
                mongo_main_db_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("set")) && (mElem->GetText()) )
                mongo_main_set_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("slave")) && (mElem->GetText()) )
                mongo_main_slave_ok_ = strncmp(mElem->GetText(),"false", 5) > 0 ? false : true;
        }

        if( (mel = mels->FirstChildElement("log")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                mongo_log_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("db")) && (mElem->GetText()) )
                mongo_log_db_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("set")) && (mElem->GetText()) )
                mongo_log_set_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("slave")) && (mElem->GetText()) )
                mongo_log_slave_ok_ = strncmp(mElem->GetText(),"false", 5) > 0 ? false : true;
        }

    }

    if( (mels = mRoot->FirstChildElement("redis")) )
    {
        if( (mel = mels->FirstChildElement("short_term_history")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                redis_short_term_history_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("port")) && (mElem->GetText()) )
                redis_short_term_history_port_ = mElem->GetText();
        }
        if( (mel = mels->FirstChildElement("long_term_history")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                redis_long_term_history_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("port")) && (mElem->GetText()) )
                redis_long_term_history_port_ = mElem->GetText();
        }
        if( (mel = mels->FirstChildElement("user_view_history")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                redis_user_view_history_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("port")) && (mElem->GetText()) )
                redis_user_view_history_port_ = mElem->GetText();
        }
        if( (mel = mels->FirstChildElement("page_keywords")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                redis_page_keywords_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("port")) && (mElem->GetText()) )
                redis_page_keywords_port_ = mElem->GetText();
        }
        if( (mel = mels->FirstChildElement("category")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                redis_category_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("port")) && (mElem->GetText()) )
                redis_category_port_ = mElem->GetText();
        }
        if( (mel = mels->FirstChildElement("retargeting")) )
        {
            if( (mElem = mel->FirstChildElement("host")) && (mElem->GetText()) )
                redis_retargeting_host_ = mElem->GetText();

            if( (mElem = mel->FirstChildElement("port")) && (mElem->GetText()) )
                redis_retargeting_port_ = mElem->GetText();
        }

    }

    if( (mElem = mRoot->FirstChildElement("range")) )
    {
        if( (mel = mElem->FirstChildElement("short_term")) && (mel->GetText()) )
            range_short_term_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("long_term")) && (mel->GetText()) )
            range_long_term_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("context")) && (mel->GetText()) )
            range_context_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("search")) && (mel->GetText()) )
            range_search_ = ::atof(mel->GetText());
    }


    if( (mElem = mRoot->FirstChildElement("server")) )
    {
        if( (mel = mElem->FirstChildElement("socket_path")) && (mel->GetText()) )
            server_socket_path_ = mel->GetText();
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
            template_teaser_ = mel->GetText();
        else
        {
            Log::warn("element template_teaser is not inited");
        }

        if( (mel = mElem->FirstChildElement("template_banner")) && (mel->GetText()) )
            template_banner_ = mel->GetText();
        else
        {
            Log::warn("element template_banner is not inited");
        }

        if( (mel = mElem->FirstChildElement("swfobject")) && (mel->GetText()) )
            swfobject_ = mel->GetText();
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
    }


    pDb = new DataBase(true);

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
