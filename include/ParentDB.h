#ifndef PARENTDB_H
#define PARENTDB_H

#include <mongo/client/dbclient_rs.h>

#include "KompexSQLiteDatabase.h"
#include "KompexSQLiteException.h"

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

    /** \brief Загружает информацию в локальную бд
        обо всех кампаниях если фильтр - по умолчанию
        или о тех что заданы в фильтре
        \param[in] фильтр mongo
    */
    void CampaignsLoadAll(mongo::Query=mongo::Query());
    /** \brief Обновить информацию в локальной бд (параметр 1) о кампании (параметр 2)*/
    void CampaignUpdate(const std::string &aCampaignId);
    /** \brief Остановить/запустить(парамерт 3) кампанию(параметр 2) guid*/
    void CampaignStartStop(const std::string &aCampaignId, int StartStop);
    /** \brief Удалить кампанию(параметр 2) из локальной бд(параметр 1)*/
    void CampaignRemove(const std::string &aCampaignId);
    std::string CampaignGetName(long long campaign_id);

private:
    bool fConnectedToMainDatabase;
    Kompex::SQLiteDatabase *pdb;
    char buf[262144];
    mongo::DBClientReplicaSet *monga_main;

    void logDb(const Kompex::SQLiteException &ex) const;


    bool ConnectMainDatabase();
    long long insertAndGetDomainId(const std::string &domain);
    long long insertAndGetAccountId(const std::string &accout);
    void GeoRerionsAdd(const std::string &country, const std::string &region);
};

#endif // PARENTDB_H
