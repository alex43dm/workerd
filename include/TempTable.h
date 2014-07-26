#ifndef TEMPTABLE_H
#define TEMPTABLE_H

#include <string>

class TempTable
{
    public:
        TempTable(char *, size_t);
        virtual ~TempTable();

        bool clear();
        const char* str() const { return tmpTableName.c_str(); }

    protected:
    private:
        std::string tmpTableName;
        char *cmd;
        size_t len;
};

#endif // TEMPTABLE_H
