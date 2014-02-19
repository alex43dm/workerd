#include "Offer.h"
#include "Log.h"
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

long long int Offer::gen()
{
    token_int = (long long int)rand();
    token = std::to_string(token_int);
    return token_int;
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

