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
        if( (mel = mElem->FirstChildElement("query")) && (mel->GetText()) )
            range_query_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("short_term")) && (mel->GetText()) )
            range_short_term_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("long_term")) && (mel->GetText()) )
            range_long_term_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("context")) && (mel->GetText()) )
            range_context_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("context_term")) && (mel->GetText()) )
            range_context_term_ = ::atof(mel->GetText());
        if( (mel = mElem->FirstChildElement("on_places")) && (mel->GetText()) )
            range_on_places_ = ::atof(mel->GetText());

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
        else
        {
            Log::warn("element dbpath is not inited: :memory:");
            dbpath_ = ":memory:";
        }

        if( (mel = mElem->FirstChildElement("swfobject_js")) && (mel->GetText()) )
            swfobject_js_ = mel->GetText();
        else
        {
            Log::warn("element swfobject_js is not inited");
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
