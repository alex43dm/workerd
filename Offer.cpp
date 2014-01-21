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
    branch(EBranchL::LMAX),
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
    /*
    std::ostringstream s;
    s << std::hex << rand(); // Cлучайное число в шестнадцатиричном
    token = s.str();  // исчислении
    */
    token = std::to_string(rand());
}


bool Offer::setBranch(const EBranchT tbranch)
{
    switch(tbranch)
    {
        case EBranchT::T1:
            switch(type)
            {
                case Type::banner:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L7;
                            return true;
                        case false:
                            branch = EBranchL::L2;
                            rating = 1000 * rating;
                            return true;
                    }
                    break;
                case Type::teazer:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L17;
                            return true;
                        case false:
                            branch = EBranchL::L12;
                            rating = 1000 * rating;
                            return true;
                    }
                    break;
                default:
                    return false;
            }
            break;
        case EBranchT::T2:
            switch(type)
            {
                case Type::banner:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L8;
                            return true;
                        case false:
                            branch = EBranchL::L3;
                            rating = 1000 * rating;
                            return true;
                    }
                    break;
                case Type::teazer:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L18;
                            return true;
                        case false:
                            branch = EBranchL::L13;
                            return true;
                    }
                    break;
                default:
                    return false;
            }
            break;
        case EBranchT::T3:
            switch(type)
            {
                case Type::banner:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L4;
                            rating = 1000 * rating;
                            return true;
                        case false:
                            branch = EBranchL::L3;
                            rating = 1000 * rating;
                            return true;
                    }
                    break;
                case Type::teazer:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L19;
                            return true;
                        case false:
                            branch = EBranchL::L14;
                            return true;
                    }
                    break;
                default:
                    return false;
            }
            break;
        case EBranchT::T4:
            switch(type)
            {
                case Type::banner:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L10;
                            return true;
                        case false:
                            branch = EBranchL::L5;
                            rating = 1000 * rating;
                            return true;
                    }
                    break;
                case Type::teazer:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L20;
                            return true;
                        case false:
                            branch = EBranchL::L15;
                            return true;
                    }
                    break;
                default:
                    return false;
            }
            break;
        case EBranchT::T5:
            switch(type)
            {
                case Type::banner:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L11;
                            return true;
                        case false:
                            branch = EBranchL::L6;
                            rating = 1000 * rating;
                            return true;
                    }
                    break;
                case Type::teazer:
                    switch(isOnClick)
                    {
                        case true:
                            branch = EBranchL::L21;
                            return true;
                        case false:
                            branch = EBranchL::L16;
                            return true;
                    }
                    break;
                default:
                    return false;
            }
            break;
        case EBranchT::TMAX:
            return false;
    }

    return false;
}

void Offer::remove(Kompex::SQLiteDatabase *pdb, const std::string &id)
{
    Kompex::SQLiteStatement *pStmt;
    char buf[8192];

    if(id.empty())
    {
        return;
    }

    pStmt = new Kompex::SQLiteStatement(pdb);
    pStmt->BeginTransaction();
    sqlite3_snprintf(sizeof(buf),buf,"DELETE FROM Offer WHERE id=%s;",id.c_str());
        try
        {
            pStmt->SqlStatement(buf);
        }
        catch(Kompex::SQLiteException &ex)
        {
            Log::err("Offer::remove(%s) error: %s", buf, ex.GetString().c_str());
        }
    pStmt->CommitTransaction();
    pStmt->FreeQuery();

    delete pStmt;

    Log::info("offer %s removed",id.c_str());
}
