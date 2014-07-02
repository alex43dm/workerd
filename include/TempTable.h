#ifndef TEMPTABLE_H
#define TEMPTABLE_H

#include <string>

class TempTable
{
    public:
        TempTable();
        virtual ~TempTable();
        bool clearTable();
        const char* tmpTable() const { return tmpTableName.c_str(); }
    protected:
    private:
        std::string tmpTableName;
        char *cmd;
};

#endif // TEMPTABLE_H
