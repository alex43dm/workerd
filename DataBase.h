#ifndef DATABASE_H
#define DATABASE_H

#include <string>

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
protected:

private:
    bool create;
    SQLiteStatement *pStmt;

    bool openDb();
    long fileSize(int fd);
    void readDir(const std::string &dirName);
};

#endif // DATABASE_H
