#include <iostream>
#include <fstream>

#include <boost/filesystem.hpp>

#include <string.h>
#include <stdlib.h>
#include <pwd.h>

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

void Config::exit(const std::string &mes)
{
    std::cerr<<mes<<std::endl;
    Log::err(mes);
    ::exit(1);
}

bool Config::Load()
{
    TiXmlElement *mel, *mels;
    boost::filesystem::path p;

    p = boost::filesystem::path(mFileName);

    if(!boost::filesystem::is_regular_file(p))
    {
        exit("does not regular file: "+mFileName);
    }

    Log::info("Config::Load: load file: %s",mFileName.c_str());

    mIsInited = false;
    mDoc = new TiXmlDocument(mFileName);

    if(!mDoc)
    {
        exit("does not found config file: "+mFileName);
    }

    if(!mDoc->LoadFile())
    {
        exit("load file: "+mFileName+
             " error: "+ mDoc->ErrorDesc()+
             " row: "+std::to_string(mDoc->ErrorRow())+
             " col: "+std::to_string(mDoc->ErrorCol()));
    }

    if(p.has_parent_path())
    {
        cfgFilePath = p.parent_path().string() + "/";
    }
    else
    {
        cfgFilePath = "./";
    }
#ifdef DEBUG
    std::cout<<"config path: "<<cfgFilePath<<std::endl;
#endif // DEBUG

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

            if(!checkPath(geocity_path_, false, true, mes))
            {
                exit(mes);
            }
        }

        if( (mel = mElem->FirstChildElement("socket_path")) && (mel->GetText()) )
        {
            server_socket_path_ = mel->GetText();

            if(!checkPath(server_socket_path_, true, true, mes))
            {
                Log::warn("server socket path: %s",mes.c_str());
            }
            else
            {
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

                if(dbpath_!=":memory:" && !checkPath(dbpath_,true, true, mes))
                {
                    exit(mes);
                }
            }
            else
            {
                Log::warn("element db is not inited: :memory:");
                dbpath_ = ":memory:";
            }

            if( (mels = mel->FirstChildElement("schema")) && (mels->GetText()) )
            {
                db_dump_path_ = cfgFilePath + mels->GetText();

                if(!checkPath(db_dump_path_,false, false, mes))
                {
                    exit(mes);
                }
            }

            if( (mels = mel->FirstChildElement("geo_csv")) && (mels->GetText()) )
            {
                db_geo_csv_ = cfgFilePath + mels->GetText();

                if(!checkPath(db_geo_csv_, false, true, mes))
                {
                    exit(mes);
                }
            }
        }

        if( (mel = mElem->FirstChildElement("lock_file")) && (mel->GetText()) )
        {
            lock_file_ = mel->GetText();

            if(!checkPath(lock_file_,true, true, mes))
            {
                exit(mes);
            }
        }

        if( (mel = mElem->FirstChildElement("pid_file")) && (mel->GetText()) )
        {
            pid_file_ = mel->GetText();

            if(!checkPath(pid_file_,true, true, mes))
            {
                exit(mes);
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

        if( (mel = mElem->FirstChildElement("time_update")) && (mel->GetText()) )
        {
            time_update_ = getTime(mel->GetText());
        }


        if( (mel = mElem->FirstChildElement("templates")) )
        {
            if( (mels = mel->FirstChildElement("teaser")) && (mels->GetText()) )
            {
                if(!checkPath(cfgFilePath + mels->GetText(),false, true, mes))
                {
                    exit(mes);
                }

                template_teaser_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element template_teaser is not inited");
            }

            if( (mels = mel->FirstChildElement("banner")) && (mels->GetText()) )
            {
                if(!checkPath(cfgFilePath + mels->GetText(),false, true, mes))
                {
                    exit(mes);
                }

                template_banner_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element template_banner is not inited");
            }

            if( (mels = mel->FirstChildElement("error")) && (mels->GetText()) )
            {
                if(!checkPath(cfgFilePath + mels->GetText(),false, true, mes))
                {
                    exit(mes);
                }

                template_error_ = getFileContents(cfgFilePath + mels->GetText());
            }
            else
            {
                exit("element template_error is not inited");
            }


            if( (mels = mel->FirstChildElement("swfobject")) && (mels->GetText()) )
            {
                if(!checkPath(cfgFilePath + mels->GetText(),false, true, mes))
                {
                    exit(mes);
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
                Log::warn("element cookie_name is not inited");
            }
            if( (mels = mel->FirstChildElement("domain")) && (mels->GetText()) )
                cookie_domain_ = mels->GetText();
            else
            {
                Log::warn("element cookie_domain is not inited");
            }

            if( (mels = mel->FirstChildElement("path")) && (mels->GetText()) )
                cookie_path_ = mels->GetText();
            else
            {
                Log::warn("element cookie_path is not inited");
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
    else
    {
        exit("no retargeting section in config file. exit");
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
    else
    {
        exit("no sphinx section in config file. exit");
    }

#endif // DUMMY

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
//---------------------------------------------------------------------------------------------------------------
/*
                stat = boost::filesystem::status(test, errcode);
                if(errcode)
                {
                  Log::err("file system error: object: %s value: %d message: %s",
                           test.string().c_str(),
                           errcode.value(),
                           errcode.message().c_str());
                    return false;
                }

*/
bool Config::checkPath(const std::string &path_, bool checkWrite, bool isFile, std::string &mes)
{
    boost::filesystem::path path, test;
    boost::system::error_code errcode;
    boost::filesystem::path::iterator toEnd;
    struct stat info;
    uid_t uid;
    gid_t gid;

    uid = getuid();
    gid = getgid();


    path = boost::filesystem::path(path_);

    toEnd = path.end();
    if(isFile)
    {
        toEnd--;
    }

    for (boost::filesystem::path::iterator it = path.begin(); it != toEnd; ++it)
    {
        test /= *it;

        if(test.string()=="" || test.string()=="/")
            continue;

        if(boost::filesystem::exists(test))
        {
            ::stat(test.string().c_str(), &info);

            if (boost::filesystem::is_regular_file(test))
            {
                if(info.st_uid != uid && info.st_gid != gid)
                {
                    if(info.st_mode &(S_IROTH | S_IWOTH))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                else if(info.st_uid == uid && info.st_gid != gid)
                {
                    if(info.st_mode & (S_IRUSR | S_IWUSR))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                else if(info.st_uid != uid && info.st_gid == gid)
                {
                    if(info.st_mode & (S_IRGRP | S_IWGRP))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                else
                {
                    if(info.st_mode & (S_IRUSR | S_IWUSR))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
            }
            else if (boost::filesystem::is_directory(test))
            {
                if(info.st_uid != uid && info.st_gid != gid)
                {
                    if(info.st_mode & (S_IROTH | S_IXOTH))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                else if(info.st_uid == uid && info.st_gid != gid)
                {
                    if(info.st_mode & (S_IRUSR | S_IXUSR))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                else if(info.st_uid != uid && info.st_gid == gid)
                {
                    if(info.st_mode & (S_IRGRP | S_IXGRP))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                else
                {
                    if(info.st_mode & (S_IRUSR | S_IXUSR))
                    {
                        continue;
                    }
                    else
                    {
                        mes = "file system error: path: "+test.string()+" message: cann't write";
                        return false;
                    }
                }
                continue;
            }
            else
            {
                if(checkWrite)
                {
                    try
                    {
                        if(!boost::filesystem::create_directories(test))
                        {
                            return false;
                        }
                    }
                    catch(const boost::filesystem::filesystem_error &ex)
                    {
                        mes = "file system error: path: "+test.string()+" message: "+ex.what();
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
        }
        else//does not exists
        {
            if(checkWrite)
            {
                try
                {
                    if(!boost::filesystem::create_directories(test))
                    {
                        return false;
                    }
                }
                catch(const boost::filesystem::filesystem_error &ex)
                {
                    mes = "file system error: path: "+test.string()+" message: "+ex.what();
                    return false;
                }
            }
            else
            {
                mes = "file system error: path: "+test.string()+" message: does not exists";
                return false;
            }
        }
    }

    if(isFile && checkWrite)
    {
        int lfp = open((path_+"test").c_str(),O_RDWR|O_CREAT,0640);

		if(lfp < 0)
		{
			exit("unable to create file: "+path_+", "+strerror(errno));
		}
		else
        {
            close(lfp);
            unlink((path_+"test").c_str());
        }
    }

    return true;
}
