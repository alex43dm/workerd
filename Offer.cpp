#include "Offer.h"
#include "DB.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"

Offer::Offer(const std::string &id,
             long id_int,
             const std::string &title,
             const std::string &price,
             const std::string &description,
             const std::string &url,
             const std::string &image_url,
             const std::string &swf,
             const std::string &campaign_id,
             bool valid,
             bool isOnClick,
             const std::string &type,
             float rating,
             int uniqueHits,
             int height,
             int width):
    id_int(id_int),
    title(title),
    price(price),
    description(description),
    url(url),
    image_url(image_url),
    swf(swf),
    campaign_id(campaign_id),
    valid(valid),
    isOnClick(isOnClick),
    type(type),
    rating(rating),
    uniqueHits(uniqueHits),
    height(height),
    width(width),
    isBanner(type=="banner")
{
}

Offer::~Offer()
{

}
/** Загружает все товарные предложения из MongoDb */
void Offer::loadFromDatabase(Kompex::SQLiteDatabase *pdb)
{
    mongo::DB db;
    auto cursor = db.query("offer", mongo::Query());
    int skipped = 0;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *pData;
    int sz, i = 0;

    pStmt = new Kompex::SQLiteStatement(pdb);

    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO Offer(id,guid,campaignId,categoryId,accountId,rating,image,height,width,isOnClick,cost\
             ,uniqueHits,swf,description,price,url,title) VALUES(");
    sz = strlen(buf);
    pData = buf + sz;
    sz = sizeof(buf) - sz;

    pStmt->BeginTransaction();
    while (cursor->more())
    {
        mongo::BSONObj x = cursor->next();
        std::string id = x.getStringField("guid");
        if (id.empty())
        {
            skipped++;
            continue;
        }

        std::string image = x.getStringField("image");
        std::string swf = x.getStringField("swf");
        if (image.empty())
        {
            if (swf.empty())
            {
                skipped++;
                continue;
            }
        }

        bzero(pData,sz);
        sqlite3_snprintf(sz,pData,
                         "%lli,'%q',%lli,%lli,'%q',%f,'%q',%d,%d,%d,%f,%d,'%q','%q','%q','%q','%q')",
                         x.getField("guid_int").numberLong(),
                         id.c_str(),
                         x.getField("campaignId_int").numberLong(),
                         x.getField("category").numberLong(),
                         x.getStringField("accountId"),
                         mongo::DB::toFloat(x.getField("rating")),
                         x.getStringField("image"),
                         x.getIntField("height"),
                         x.getIntField("width"),
                         x.getBoolField("isOnClick") ? 1 : 0,
                         mongo::DB::toFloat(x.getField("cost")),
                         x.getIntField("uniqueHits"),
                         swf.c_str(),
                         x.getStringField("description"),
                         x.getStringField("price"),
                         x.getStringField("url"),
                         x.getStringField("title"));

        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Offers::_loadFromQuery insert(%s) error: %s", buf, ex.GetString().c_str());
            skipped++;
        }

        i++;

    }

    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"REINDEX");
    try
    {
        pStmt->SqlStatement(buf);
    }
    catch(Kompex::SQLiteException &ex)
    {
        Log::err("Offers::_loadFromQuery REINDEX(%s) error: %s", buf, ex.GetString().c_str());
        skipped++;
    }

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Loaded %d offers", i);
    if (skipped)
        Log::warn("Offers with empty id or image skipped: %d", skipped);
}
