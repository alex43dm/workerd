#ifndef PARENTDB_H
#define PARENTDB_H

#include "DB.h"
#include "KompexSQLiteDatabase.h"

class ParentDB
{
    public:
        ParentDB();
        virtual ~ParentDB();
        void loadRating(const std::string &id="");
        void OfferLoad(mongo::Query=mongo::Query());
        void OfferRemove(const std::string &id);
        void CategoriesLoad();
        bool InformerLoadAll();
        bool InformerUpdate(const std::string &id);
        void InformerRemove(const std::string &id);
    protected:
    private:
        Kompex::SQLiteDatabase *pdb;
        char buf[8192];

        long long insertAndGetDomainId(const std::string &domain);
        long long insertAndGetAccountId(const std::string &accout);
};

#endif // PARENTDB_H
