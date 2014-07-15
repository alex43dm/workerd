//#include <iostream>
#include <fstream>

#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <assert.h>

#include "Log.h"
#include "Config.h"
#include "BoostHelpers.h"


unsigned long request_processed_;
unsigned long last_time_request_processed;
unsigned long offer_processed_;
unsigned long social_processed_;

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
}

bool Config::LoadConfig(const std::string fName)
{
    mFileName = fName;
    return Load();
}

void Config::redisHostAndPort(TiXmlElement *p, std::string &host, std::string &port, unsigned &timeout)
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

        if( (t2 = t1->FirstChildElement("timeout")) && (t2->GetText()) )
        {
            timeout = strtol(t2->GetText(),NULL,10);
        }
    }
}

void Config::exit(const std::string &mes)
{
    std::cerr<<mes<<std::endl;
    std::clog<<mes<<std::endl;
    ::exit(1);
}

bool Config::Load()
{
    TiXmlDocument *mDoc;
    TiXmlElement *mRoot, *mElem, *mel, *mels;

    std::clog<<"open config file:"<<mFileName;

    mIsInited = false;

    if((cfgFilePath = BoostHelpers::getConfigDir(mFileName)).empty())
    {
        return false;
    }

    std::clog<<" config dir:"<<cfgFilePath;

    if(!(mDoc = new TiXmlDocument(mFileName)))
    {
        exit("error create TiXmlDocument object");
    }


    if(!mDoc->LoadFile())
    {
        exit("load file: "+mFileName+
             " error: "+ mDoc->ErrorDesc()+
             " row: "+std::to_string(mDoc->ErrorRow())+
             " col: "+std::to_string(mDoc->ErrorCol()));
    }


    mRoot = mDoc->FirstChildElement("root");

    if(!mRoot)
    {
        exit("does not found root section in file: "+mFileName);
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
        else
        {
            exit("no main section in mongo in config file. exit");
        }

        if( (mel = mels->FirstChildElement("log")) )
        {
            for(mElem = mel->FirstChildElement("host"); mElem; mElem = mElem->NextSiblingElement("host"))
            {
                mongo_log_host_.push_back(mElem->GetText());
            }

            if( (mElem = mel->FirstChildElement("db")) && (mElem->GetText()) )
            {
                mongo_log_db_ = mElem->GetText();
            }

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
        else
        {
            exit("no log section in mongo in config file. exit");
        }
    }
    else
    {
        exit("no mongo section in config file. exit");
    }


    //main config
    if( (mElem = mRoot->FirstChildElement("server")) )
    {
        if( (mel = mElem->FirstChildElement("ip")) && (mel->GetText()) )
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

            if(!BoostHelpers::checkPath(geocity_path_, false, true))
            {
                ::exit(1);
            }
        }

        if( (mel = mElem->FirstChildElement("socket_path")) && (mel->GetText()) )
        {
            server_socket_path_ = mel->GetText();

            if(BoostHelpers::checkPath(server_socket_path_, true, true))
            {
                std::clog<<"server socket path: "<<server_socket_path_<<" exists"<<std::endl;
                unlink(server_socket_path_.c_str());
            }
        }

        if( (mel = mElem->FirstChildElement("children")) && (mel->GetText()) )
        {
            server_children_ = atoi(mel->GetText());
        }

        if( (mel = mElem->FirstChildElement("sqlite")) )
        {
            if( (mels = mel->FirstChildElement("db")) && (mels->GetText()) )
            {
                dbpath_ = mels->GetText();

                if(dbpath_!=":memory:" && !BoostHelpers::checkPath(dbpath_,true, true))
                {
                    ::exit(1);
                }
            }
            else
            {
                std::clog<<"sqlite database mode: in memory"<<std::endl;
                dbpath_ = ":memory:";
            }

            if( (mels = mel->FirstChildElement("schema")) && (mels->GetText()) )
            {
                db_dump_path_ = cfgFilePath + mels->GetText();

                if(!BoostHelpers::checkPath(db_dump_path_,false, false))
                {
                    ::exit(1);
                }
            }

            if( (mels = mel->FirstChildElement("geo_csv")) && (mels->GetText()) )
            {
                db_geo_csv_ = cfgFilePath + mels->GetText();

                if(!BoostHelpers::checkPath(db_geo_csv_, false, true))
                {
                    ::exit(1);
                }
            }
        }

        if( (mel = mElem->FirstChildElement("lock_file")) && (mel->GetText()) )
        {
            lock_file_ = mel->GetText();

            if(!BoostHelpers::checkPath(lock_file_,true, true))
            {
                ::exit(1);
            }
        }

        if( (mel = mElem->FirstChildElement("pid_file")) && (mel->GetText()) )
        {
            pid_file_ = mel->GetText();

            if(!BoostHelpers::checkPath(pid_file_,true, true))
            {
                ::exit(1);
            }
        }

        if( (mel = mElem->FirstChildElement("mq_path")) && (mel->GetText()) )
        {
            mq_path_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("user")) && (mel->GetText()) )
        {
            user_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("group")) && (mel->GetText()) )
        {
            group_ = mel->GetText();
        }

        if( (mel = mElem->FirstChildElement("time_update")) && (mel->GetText()) )
        {
            if((time_update_ = BoostHelpers::getSeconds(mel->GetText())) == -1)
            {
                exit("Config::Load: no time match in config.xml element: time_update");
            }
        }


        if( (mel = mElem->FirstChildElement("templates")) )
        {
            if( (mels = mel->FirstChildElement("teaser")) && (mels->GetText()) )
            {
                if(!BoostHelpers::checkPath(cfgFilePath + mels->GetText(),false, true))
                {
                    ::exit(1);
                }

                template_teaser_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element template_teaser is not inited");
            }

            if( (mels = mel->FirstChildElement("banner")) && (mels->GetText()) )
            {
                if(!BoostHelpers::checkPath(cfgFilePath + mels->GetText(),false, true))
                {
                    ::exit(1);
                }

                template_banner_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element template_banner is not inited");
            }

            if( (mels = mel->FirstChildElement("error")) && (mels->GetText()) )
            {
                if(!BoostHelpers::checkPath(cfgFilePath + mels->GetText(),false, true))
                {
                    ::exit(1);
                }

                template_error_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element template_error is not inited");
            }


            if( (mels = mel->FirstChildElement("swfobject")) && (mels->GetText()) )
            {
                if(!BoostHelpers::checkPath(cfgFilePath + mels->GetText(),false, true))
                {
                    ::exit(1);
                }

                swfobject_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element swfobject is not inited");
            }
        }

        if( (mel = mElem->FirstChildElement("cookie")) )
        {
            if( (mels = mel->FirstChildElement("name")) && (mels->GetText()) )
                cookie_name_ = mels->GetText();
            else
            {
                exit("element cookie_name is not inited");
            }
            if( (mels = mel->FirstChildElement("domain")) && (mels->GetText()) )
                cookie_domain_ = mels->GetText();
            else
            {
                exit("element cookie_domain is not inited");
            }

            if( (mels = mel->FirstChildElement("path")) && (mels->GetText()) )
                cookie_path_ = mels->GetText();
            else
            {
                exit("element cookie_path is not inited");
            }
        }
    }
    else
    {
        exit("no server section in config file. exit");
    }

#ifndef DUMMY
    //retargeting config
    if( (mElem = mRoot->FirstChildElement("retargeting")) )
    {
        if( (mel = mElem->FirstChildElement("percentage")) && (mel->GetText()) )
        {
            retargeting_percentage_ = strtol(mel->GetText(),NULL,10);
        }

        if( (mel = mElem->FirstChildElement("ttl")) && (mel->GetText()) )
        {
            retargeting_by_time_ = getTime(mel->GetText());
        }
        else
        {
            retargeting_by_time_ = 24*3600;
        }

        redisHostAndPort(mElem, redis_retargeting_host_, redis_retargeting_port_, redis_retargeting_timeout_);
    }
    else
    {
        exit("no retargeting section in config file. exit");
    }

    //history
    TiXmlElement *history, *section;

    range_short_term_ = 0.0;
    range_long_term_ = 0.0;
    range_context_ = 0.0;
    range_search_ = 0.0;

    if( (history = mRoot->FirstChildElement("history")) )
    {
        if( (mel = history->FirstChildElement("offer_by_campaign_unique")) && (mel->GetText()) )
        {
            offer_by_campaign_unique_ = (unsigned)strtol(mel->GetText(),NULL,10);
        }
        else
        {
            offer_by_campaign_unique_ = 1;
        }

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

            redisHostAndPort(section, redis_user_view_history_host_, redis_user_view_history_port_,redis_user_view_history_timeout_);
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

            redisHostAndPort(section, redis_short_term_history_host_, redis_short_term_history_port_,redis_short_term_history_timeout_);
        }
        //long term
        if( (section = history->FirstChildElement("long_term")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                range_long_term_ = ::atof(mel->GetText());
            }

            redisHostAndPort(section, redis_long_term_history_host_, redis_long_term_history_port_,redis_long_term_history_timeout_);
        }
        //context
        if( (section = history->FirstChildElement("context")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                range_context_ = ::atof(mel->GetText());
            }
        }
        //context
        if( (section = history->FirstChildElement("search")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                range_search_ = ::atof(mel->GetText());
            }
        }
    }
    else
    {
        exit("no history section in config file. exit");
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

            sphinx_field_len_ = 0;
            for(mElem = mel->FirstChildElement(); mElem; mElem = mElem->NextSiblingElement(),sphinx_field_len_++);

            sphinx_field_names_ = (const char**)malloc(sphinx_field_len_ * sizeof(const char*));
            sphinx_field_weights_ = (int *)malloc(sizeof(int*));
            sphinx_field_len_ = 0;
            for(mElem = mel->FirstChildElement(); mElem; mElem = mElem->NextSiblingElement(),sphinx_field_len_++)
            {
                sphinx_field_names_[sphinx_field_len_] = strdup(mElem->Value());
                sphinx_field_weights_[sphinx_field_len_] = atoi(mElem->GetText());

                std::clog<<"sphinx field name: "<<sphinx_field_names_[sphinx_field_len_]
                <<", weights: "<<sphinx_field_weights_[sphinx_field_len_]<<std::endl;
            }
        }
        std::clog<<"sphinx mach mode: "<<shpinx_match_mode_<<", rank mode: "<<shpinx_rank_mode_<<" sort mode: "<<shpinx_sort_mode_<<std::endl;
    }
    else
    {
        exit("no sphinx section in config file. exit");
    }

#endif // DUMMY

    if( (mElem = mRoot->FirstChildElement("log")) )
    {
        if( (mel = mElem->FirstChildElement("coretime")) && (mel->GetText()) )
        {
            logCoreTime = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("outsize")) && (mel->GetText()) )
        {
            logOutPutSize = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("ip")) && (mel->GetText()) )
        {
            logIP = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("country")) && (mel->GetText()) )
        {
            logCountry = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("region")) && (mel->GetText()) )
        {
            logRegion = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("cookie")) && (mel->GetText()) )
        {
            logCookie = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("context")) && (mel->GetText()) )
        {
            logContext = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("search")) && (mel->GetText()) )
        {
            logSearch = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("informerId")) && (mel->GetText()) )
        {
            logInformerId = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("location")) && (mel->GetText()) )
        {
            logLocation = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("sphinx")) && (mel->GetText()) )
        {
            logSphinx = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("RetargetingOfferIds")) && (mel->GetText()) )
        {
            logRetargetingOfferIds = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("OutPutOfferIds")) && (mel->GetText()) )
        {
            logOutPutOfferIds = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }
    }
    else
    {
        logCoreTime = logOutPutSize = logIP = true;

        logSphinx = logOutPutOfferIds = logRetargetingOfferIds =
        logLocation = logInformerId = logSearch =
        logContext = logCookie = logCountry = logRegion = false;

        std::clog<<"using default log mode: CoreTime OutPutSize IP"<<std::endl;
    }

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

    std::clog<<"error open file: "<<fileName<<" error number: "<<errno<<std::endl;
    return std::string();
}

bool Config::ReLoad()
{
    TiXmlDocument *mDoc;
    TiXmlElement *mRoot, *mElem, *mel;
    bool returnFlag = false;

    mDoc = new TiXmlDocument(mFileName);

    if(!mDoc)
    {
        std::clog<<"error open config file:"<<mFileName<<std::endl;
        return false;
    }

    std::clog<<"reload config file:"<<mFileName<<" path: "<<cfgFilePath<<std::endl;

    if(!mDoc->LoadFile())
    {
        std::clog<<" config file:"<<mFileName<<" is not valid."
             <<" error: "<<mDoc->ErrorDesc()
             <<" row: "<<mDoc->ErrorRow()
             <<" col: "<<mDoc->ErrorCol();
        goto clear_obj;
    }


    mRoot = mDoc->FirstChildElement("root");

    if(!mRoot)
    {
        std::clog<<"does not found root section"<<std::endl;
        goto clear_obj;
    }

    if( (mElem = mRoot->FirstChildElement("log")) )
    {
        if( (mel = mElem->FirstChildElement("coretime")) && (mel->GetText()) )
        {
            logCoreTime = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("outsize")) && (mel->GetText()) )
        {
            logOutPutSize = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("ip")) && (mel->GetText()) )
        {
            logIP = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("country")) && (mel->GetText()) )
        {
            logCountry = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("region")) && (mel->GetText()) )
        {
            logRegion = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("cookie")) && (mel->GetText()) )
        {
            logCookie = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("context")) && (mel->GetText()) )
        {
            logContext = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("search")) && (mel->GetText()) )
        {
            logSearch = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("informerId")) && (mel->GetText()) )
        {
            logInformerId = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("location")) && (mel->GetText()) )
        {
            logLocation = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("sphinx")) && (mel->GetText()) )
        {
            logSphinx = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("RetargetingOfferIds")) && (mel->GetText()) )
        {
            logRetargetingOfferIds = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }

        if( (mel = mElem->FirstChildElement("OutPutOfferIds")) && (mel->GetText()) )
        {
            logOutPutOfferIds = strncmp(mel->GetText(),"1",1)>=0 ? true : false;
        }
    }
    else
    {
        std::clog<<"does not found log section"<<std::endl;
        goto clear_obj;
    }

    returnFlag = true;

clear_obj:
    delete mDoc;

    return returnFlag;
}

bool Config::Save()
{
    TiXmlDocument *mDoc;
    TiXmlElement *mRoot;
    bool returnFlag = false;

    mDoc = new TiXmlDocument(mFileName);

    if(!mDoc)
    {
        std::clog<<"error open config file:"<<mFileName<<std::endl;
        return false;
    }

    std::clog<<"reload config file:"<<mFileName<<" path: "<<cfgFilePath<<std::endl;

    if(!mDoc->LoadFile())
    {
        std::clog<<" config file:"<<mFileName<<" is not valid."
                 <<" error: "<<mDoc->ErrorDesc()
                 <<" row: "<<mDoc->ErrorRow()
                 <<" col: "<<mDoc->ErrorCol();
        goto clear_obj;
    }


    mRoot = mDoc->FirstChildElement("root");

    if(!mRoot)
    {
        std::clog<<"does not found root section"<<std::endl;
        goto clear_obj;
    }


    //history
    TiXmlElement *history, *section, *mel;

    if( (history = mRoot->FirstChildElement("history")) )
    {
        if( (mel = history->FirstChildElement("offer_by_campaign_unique")))
        {
            mel->SetValue(std::to_string(offer_by_campaign_unique_));
        }

        //short term
        if( (section = history->FirstChildElement("short_term")) )
        {
            if( (mel = section->FirstChildElement("value")) )
            {
                mel->SetValue(BoostHelpers::float2string(range_short_term_));
            }
        }

        //long term
        if( (section = history->FirstChildElement("long_term")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                mel->SetValue(BoostHelpers::float2string(range_long_term_));
            }
        }
        //context
        if( (section = history->FirstChildElement("context")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                mel->SetValue(BoostHelpers::float2string(range_context_));
            }
        }
        //context
        if( (section = history->FirstChildElement("search")) )
        {
            if( (mel = section->FirstChildElement("value")) && (mel->GetText()) )
            {
                mel->SetValue(BoostHelpers::float2string(range_search_));
            }
        }

        returnFlag = true;

        mDoc->SaveFile();
    }
    else
    {
        std::clog<<"no history section in config file"<<std::endl;
    }

clear_obj:
    delete mDoc;

    return returnFlag;
}
