#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteStatement.h"

using namespace Kompex;

class DataBase
{
public:
    SQLiteDatabase *pDatabase;

    DataBase(bool create = false);
    virtual ~DataBase();
    bool gen(int from, unsigned int len);
    void postDataLoad();
    void indexRebuild();
    bool runSqlFile(const std::string &file);
    std::string getSqlFile(const std::string &file);
protected:

private:
    std::string dbFileName;
    std::string dirName;
    std::string geoCsv;
    bool create;
    SQLiteStatement *pStmt;

    bool openDb();
    long fileSize(int fd);
    void readDir(const std::string &dname);
    bool runSqlFiles(const std::vector<std::string> &files);
};

#endif // DATABASE_H
