#include <iostream>
#include <algorithm>
#include <fstream>

#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>

#include "Log.h"
#include "DataBase.h"
#include "KompexSQLiteException.h"
#include "GeoRerions.h"
#include "Config.h"

#define INSERTSTATMENT "INSERT INTO Offer (id) VALUES (%lu)"

bool is_file_exist(const std::string &fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

DataBase::DataBase(bool create) :
    reopen(false),
    dbFileName(Config::Instance()->dbpath_),
    dirName(Config::Instance()->db_dump_path_),
    geoCsv(Config::Instance()->db_geo_csv_),
    create(create),
    pStmt(nullptr)
{
    openDb();
}

DataBase::~DataBase()
{
    pStmt->FreeQuery();

    if(pStmt)
    {
        delete pStmt;
    }
}

bool DataBase::openDb()
{
    int flags;

    try
    {
        if(pStmt)
        {
            delete pStmt;
        }

        if(!create)
        {
            pDatabase = new Kompex::SQLiteDatabase(dbFileName,
                                           SQLITE_OPEN_READONLY, NULL);//SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | | SQLITE_OPEN_FULLMUTEX

            pStmt = new Kompex::SQLiteStatement(pDatabase);

            return true;
        }

        Log::gdb("open and create db");

        flags = SQLITE_OPEN_READWRITE;

        reopen = is_file_exist(dbFileName);

        if(!reopen)
        {
            flags |= SQLITE_OPEN_CREATE;
        }

        pDatabase = new Kompex::SQLiteDatabase(dbFileName, flags, NULL);//SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | | SQLITE_OPEN_FULLMUTEX

        pStmt = new Kompex::SQLiteStatement(pDatabase);

        if(reopen)
        {
            return true;
        }

        //load db dump from directory

        readDir(dirName + "/tables");
        readDir(dirName + "/view");

        //geo table fill
        GeoRerions::load(pDatabase,geoCsv);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        exit(1);
    }

    return true;
}

bool DataBase::gen(int from, unsigned int len)
{
    char buf[2048];

    try
    {
        pStmt->BeginTransaction();

        for(unsigned long i = 0; i < len; i++)
        {
            bzero(buf,sizeof(buf));
            snprintf(buf,sizeof(buf),INSERTSTATMENT, from + i);
            pStmt->SqlStatement(buf);
        }

        pStmt->CommitTransaction();
    }
    catch(Kompex::SQLiteException &ex)
    {
        printf("DataBase DB error: %s", ex.GetString().c_str());
    }
    return true;
}

long DataBase::fileSize(int fd)
{
    struct stat stat_buf;
    return fstat(fd, &stat_buf) == 0 ? stat_buf.st_size : -1;
}

void DataBase::readDir(const std::string &dname)
{
    struct dirent *sql_name;

    DIR *dir = opendir(dname.c_str());

    if (dir == NULL)
    {
        Log::err("DataBase::readDir: scandir: %s error",dname.c_str());
        return;
    }
    std::vector<std::string> files;
    while( (sql_name = readdir(dir)) != NULL )
    {
        if(sql_name->d_type != DT_REG)
        {
            Log::warn("DataBase::readDir: alien file %s does not included!", sql_name->d_name);
            continue;
        }

        if(strstr(sql_name->d_name, ".sql") != NULL)
        {
            files.push_back(dname + "/" + sql_name->d_name);
            Log::info("DataBase::readDir: add file: %s", sql_name->d_name);
        }
        else
        {
            Log::warn("DataBase::readDir: file %s does not included!", sql_name->d_name);
        }
    }
    closedir(dir);
    std::sort (files.begin(), files.end());
    runSqlFiles(files);
}

bool DataBase::runSqlFiles(const std::vector<std::string> &files)
{
    for (auto it = files.begin(); it != files.end(); it++)
    {
        runSqlFile(*it);
    }

    return true;
}


void DataBase::postDataLoad()
{
    try
    {
        pStmt->BeginTransaction();

        readDir(dirName + "/post");

        pStmt->CommitTransaction();
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DataBase DB error: postDataLoad: %s", ex.GetString().c_str());
    }
}

//The REINDEX command is used to delete and recreate indices from scratch.
void DataBase::indexRebuild()
{
    try
    {
//        pStmt->BeginTransaction();
        pStmt->SqlStatement("REINDEX");
//        pStmt->CommitTransaction();
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DataBase DB error: indexRebuild: %s", ex.GetString().c_str());
    }
}

bool DataBase::runSqlFile(const std::string &file)
{
    int fd;

    if( (fd = open(file.c_str(), O_RDONLY))<2 )
    {
        Log::err("DataBase::runSqlFile: error open %s", file.c_str());
        return false;
    }

    ssize_t sz = fileSize(fd);
    char *buf = (char*)malloc(sz);

    bzero(buf,sz);

    int ret = read(fd, buf, sz);

    if( ret != sz )
    {
        printf("Error read file: %s",file.c_str());
        ::exit(1);
    }
    close(fd);

    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        printf("error in file: %s: %s\n", file.c_str(), ex.GetString().c_str());
        ::exit(1);
    }
    char *p = buf;
    while(p < buf+sz)
    {
        if(*p==';' && p < buf+sz-30)
        {
            pStmt->SqlStatement(++p);
        }
        else
        {
            p++;
        }
    }

    free(buf);
    Log::err("DataBase::runSqlFile: run %s", file.c_str());
    return true;
}

std::string DataBase::getSqlFile(const std::string &file)
{
    int fd;
    std::string retString;

    if( (fd = open((dirName + "/" + file).c_str(), O_RDONLY))<2 )
    {
        throw "error open: " + dirName + "/" + file;
    }
    ssize_t sz = fileSize(fd);
    char *buf = (char*)malloc(sz);

    bzero(buf,sz);

    int ret = read(fd, buf, sz);

    if( ret != sz )
    {
        printf("Error read file: %s",(dirName + "/" + file).c_str());
        ::exit(1);
    }
    close(fd);

    retString = std::string(buf,ret);

    free(buf);

    return retString;
}

void DataBase::exec(const std::string &sql)
{
   try
    {
        pStmt->SqlStatement(sql);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("DB error: cmd: %s msg: %s", sql.c_str(), ex.GetString().c_str());
    }
}
