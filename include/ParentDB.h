#ifndef PARENTDB_H
#define PARENTDB_H

#include "DB.h"
#include "KompexSQLiteDatabase.h"

class ParentDB
{
    public:
        ParentDB();
        virtual ~ParentDB();
        void loadRating(bool isClear);
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
        long long GeoRerionsAdd(const std::string &country, const std::string &region);
};

#endif // PARENTDB_H
