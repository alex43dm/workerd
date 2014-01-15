#include "Offer.h"
#include "DB.h"
#include "Log.h"
#include "KompexSQLiteStatement.h"
#include "KompexSQLiteException.h"
#include "json.h"

Offer::Offer(const std::string &id,
             long long id_int,
             const std::string &title,
             const std::string &price,
             const std::string &description,
             const std::string &url,
             const std::string &image_url,
             const std::string &swf,
             long long campaign_id,
             bool valid,
             bool isOnClick,
             int type,
             float rating,
             bool retargeting,
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
    type(typeFromInt(type)),
    rating(rating),
    retargeting(retargeting),
    uniqueHits(uniqueHits),
    height(height),
    width(width),
    isBanner(this->type==Offer::Type::banner)
{
}

Offer::~Offer()
{

}
/** Загружает все товарные предложения из MongoDb */
void Offer::loadAll(Kompex::SQLiteDatabase *pdb, mongo::Query q_correct)
{
    mongo::DB db;
    Kompex::SQLiteStatement *pStmt;
    char buf[8192], *pData;
    int sz, i = 0;
    int skipped = 0;

    auto cursor = db.query("offer", q_correct);

    pStmt = new Kompex::SQLiteStatement(pdb);


    bzero(buf,sizeof(buf));
    snprintf(buf,sizeof(buf),"INSERT INTO Offer(id,guid,campaignId,categoryId,accountId,rating,retargeting,image,height,width,isOnClick,cost\
             ,uniqueHits,swf,description,price,url,title,type,valid) VALUES(");
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
                         "%lli,'%q',%lli,%lli,'%q',%f,%d,'%q',%d,%d,%d,%f,%d,'%q','%q','%q','%q','%q',%d,%d);",
                         x.getField("guid_int").numberLong(),
                         id.c_str(),
                         x.getField("campaignId_int").numberLong(),
                         x.getField("category").numberLong(),
                         x.getStringField("accountId"),
                         mongo::DB::toFloat(x.getField("rating")),
                         x.getBoolField("retargeting") ? 1 : 0,
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
                         x.getStringField("title"),
                         Offer::typeFromString(x.getStringField("type")),
                         1
                        );

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

    pStmt->CommitTransaction();
    pStmt->FreeQuery();
    delete pStmt;

    Log::info("Loaded %d offers", i);
    if (skipped)
        Log::warn("Offers with empty id or image skipped: %d", skipped);
}


std::string Offer::toJson() const
{
    std::stringstream json;

    json << "{" <<
         "\"id\": \"" << id_int << "\"," <<
         "\"title\": \"" << Json::Utils::Escape(title) << "\"," <<
         "\"description\": \"" << Json::Utils::Escape(description) << "\"," <<
         "\"price\": \"" << Json::Utils::Escape(price) << "\"," <<
         "\"image\": \"" << Json::Utils::Escape(image_url) << "\"," <<
         "\"swf\": \"" << Json::Utils::Escape(swf) << "\"," <<
         "\"url\": \"" << Json::Utils::Escape(redirect_url) << "\"," <<
         "\"token\": \"" << Json::Utils::Escape(token) << "\"," <<
         "\"rating\": \"" << rating << "\"," <<
         "\"width\": \"" << width << "\"," <<
         "\"height\": \"" << height << "\"" <<
         "}";

    return json.str();
}

void Offer::gen()
{
    std::ostringstream s;
    s << std::hex << rand(); // Cлучайное число в шестнадцатиричном
    token = s.str();  // исчислении
}


bool Offer::setBranch(const std::string &qbranch)
{
    if (type == Type::banner and isOnClick == false and qbranch == "T1")
    {
        branch = "L2";
        rating = 1000 * rating;
    }
    else if (type == Type::banner and isOnClick == false and qbranch == "T2")
    {
        branch = "L3";
        rating = 1000 * rating;
    }
    else if (type == Type::banner and isOnClick == false and qbranch == "T3")
    {
        branch = "L4";
        rating = 1000 * rating;
    }
    else if (type == Type::banner and isOnClick == false and qbranch == "T4")
    {
        branch = "L5";
        rating = 1000 * rating;
    }
    else if (type == Type::banner and isOnClick == false and qbranch == "T5")
    {
        branch = "L6";
        rating = 1000 * rating;
    }
    else if (type == Type::banner and isOnClick == true and qbranch == "T1")
    {
        branch = "L7";
    }
    else if (type == Type::banner and isOnClick == true and qbranch == "T2")
    {
        branch = "L8";
    }
    else if (type == Type::banner and isOnClick == true and qbranch == "T3")
    {
        branch = "L9";
    }
    else if (type == Type::banner and isOnClick == true and qbranch == "T4")
    {
        branch = "L10";
    }
    else if (type == Type::banner and isOnClick == true and qbranch == "T5")
    {
        branch = "L11";
    }
    else if (type == Type::teazer and isOnClick == false and qbranch == "T1")
    {
        branch = "L12";
    }
    else if (type == Type::teazer and isOnClick == false and qbranch == "T2")
    {
        branch = "L13";
    }
    else if (type == Type::teazer and isOnClick == false and qbranch == "T3")
    {
        branch = "L14";
    }
    else if (type == Type::teazer and isOnClick == false and qbranch == "T4")
    {
        branch = "L15";
    }
    else if (type == Type::teazer and isOnClick == false and qbranch == "T5")
    {
        branch = "L16";
    }
    else if (type == Type::teazer and isOnClick == true and qbranch == "T1")
    {
        branch = "L17";
    }
    else if (type == Type::teazer and isOnClick == true and qbranch == "T2")
    {
        branch = "L18";
    }
    else if (type == Type::teazer and isOnClick == true and qbranch == "T3")
    {
        branch = "L19";
    }
    else if (type == Type::teazer and isOnClick == true and qbranch == "T4")
    {
        branch = "L20";
    }
    else if (type == Type::teazer and isOnClick == true and qbranch == "T5")
    {
        branch = "L21";
    }
    else
    {
        return false;
    }
    return true;
}
