#include <iostream>

#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <stdlib.h>

#include "Log.h"
#include "DataBase.h"
#include "KompexSQLiteException.h"
#include "GeoRerions.h"

#define INSERTSTATMENT "INSERT INTO Offer (id) VALUES (%lu)"

DataBase::DataBase(bool create) :
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
    try
    {
        if(!create)
        {
            pDatabase = new SQLiteDatabase(":memory:",//file::memory:",//:memory:",//"/tmp/mem.db",//?cache=shared
                                           SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, NULL);//SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | | SQLITE_OPEN_FULLMUTEX
            return true;
        }

        Log::gdb("open and create db");

        pDatabase = new SQLiteDatabase(":memory:",//file::memory:",//:memory:",//"/tmp/mem.db",//
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);//SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | | SQLITE_OPEN_FULLMUTEX

        if(pStmt)
        {
            delete pStmt;
        }
        pStmt = new SQLiteStatement(pDatabase);

        //load db dump from directory
        readDir("db_dump");
        //geo table fill
        GeoRerions::load(pDatabase);

    }
    catch(SQLiteException &ex)
    {
        Log::err("DB error: %s", ex.GetString().c_str());
        exit(1);
    }

    return true;
}
/*
bool DataBase::insCampaing(CampaignData *data)
{
    char buf[8192];

    try
    {
        pStmt->BeginTransaction();

        for(unsigned long i = 0; i < len; i++)
        {
            bzero(buf,sizeof(buf));
            snprintf(buf,sizeof(buf),
                     "INSERT INTO Campaign(id,guid,title,project,social,valid) VALUES(%lu,'%s','%s','%s',%d,%d)",
                     );
            pStmt->SqlStatement(buf);
        }

        pStmt->CommitTransaction();
    }
    catch(SQLiteException &ex)
    {
        printf("DataBase DB error: %s", ex.GetString().c_str());
    }
    return true;
}
*/
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
    catch(SQLiteException &ex)
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

void DataBase::readDir(const std::string &dirName)
{
    struct dirent *sql_name;

    for(int i=0; i<2; i++)
    {
        std::string dname = dirName;
        if( i==0 ){ dname += "/tables"; }else{ dname += "/view"; }

        DIR *dir = opendir(dname.c_str());

        if (dir == NULL)
        {
            printf("scandir error\n");
            return;
        }
        std::string fname;
        while( (sql_name = readdir(dir)) != NULL )
        {
            if(sql_name->d_type != DT_REG)
                continue;
            fname = dname + "/" + sql_name->d_name;
            int fd;
            if( (fd = open(fname.c_str(), O_RDONLY))>0 )
            {
                ssize_t sz = fileSize(fd);
                char *buf = (char*)malloc(sz);

                bzero(buf,sz);

                int ret = read(fd, buf, sz);

                if( ret != sz )
                {
                    printf("Error read file: %s",fname.c_str());
                    ::exit(1);
                }
                close(fd);

                pStmt->SqlStatement(buf);

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
            }
        }
        closedir(dir);
    }//for
}
