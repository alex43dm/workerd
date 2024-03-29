#ifndef CORE_DATABASE_H
#define CORE_DATABASE_H

#include <string>

#include "Informer.h"
#include "Offer.h"
#include "TempTable.h"

class Core_DataBase
{
    public:
        bool all_social;
        unsigned offersTotal;
        float teasersMaxRating;

        Informer *informer;

        Core_DataBase();
        virtual ~Core_DataBase();

        bool getGeo(const std::string &country, const std::string &region);
        bool getOffers(Offer::Map &items,unsigned long long sessionId);
        bool getInformer(const std::string informer_id);
        bool getCampaign();
        bool clearTmp();

    protected:
    private:
        char *cmd;
        size_t len;
        TempTable *tmpTable;
        std::string geo;
};

#endif // CORE_DATABASE_H
