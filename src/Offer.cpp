#include <sstream>

#include "Offer.h"
#include "Log.h"
#include "json.h"

Offer::Offer(const std::string &id,
             unsigned long long id_int,
             const std::string &title,
             const std::string &description,
             const std::string &url,
             const std::string &image_url,
             const std::string &swf,
             unsigned long long campaign_id,
             int type_int,
             float rating,
             int uniqueHits,
             int height,
             int width,
             bool isOnClick,
             bool social,
             std::string campaign_guid,
             int unique_by_campaign,
             bool retargeting):
    id(id),
    id_int(id_int),
    title(title),
    description(description),
    url(url),
    image_url(image_url),
    swf(swf),
    campaign_id(campaign_id),
    type(typeFromInt(type_int)),
#ifndef DUMMY
    branch(EBranchL::L30),
#else
    branch(EBranchL::L0),
#endif
    rating(rating),
    uniqueHits(uniqueHits),
    height(height),
    width(width),
    isOnClick(isOnClick),
    social(social),
    campaign_guid(campaign_guid),
    unique_by_campaign((unsigned)unique_by_campaign),
    retargeting(retargeting)
{
}

Offer::~Offer()
{

}

std::string Offer::toJson() const
{
    std::stringstream str_json;

    str_json << "{\n" <<
         "\t\"id\": \"" << id_int << "\",\n" <<
         "\t\"guid\": \"" << id << "\",\n" <<
         "\t\"title\": \"" << Json::Utils::Escape(title) << "\",\n" <<
         "\t\"description\": \"" << Json::Utils::Escape(description) << "\",\n" <<
         "\t\"price\": \"\",\n" <<
         "\t\"image\": \"" << Json::Utils::Escape(image_url) << "\",\n" <<
         "\t\"swf\": \"" << Json::Utils::Escape(swf) << "\",\n" <<
         "\t\"url\": \"" << Json::Utils::Escape(redirect_url) << "\",\n" <<
         "\t\"token\": \"" << token << "\",\n" <<
         "\t\"rating\": \"" << rating << "\",\n" <<
         "\t\"width\": \"" << width << "\",\n" <<
         "\t\"height\": \"" << height << "\",\n" <<
         "\t\"campaign_id\": \"" << campaign_id << "\",\n" <<
         "\t\"campaign_guid\": \"" << campaign_guid << "\",\n" <<
         "\t\"branch\": \"" << getBranch() << "\"\n" <<
         "}\n";

    return str_json.str();
}

std::string Offer::gen()
{
    token_int = (long long int)rand();
    token = std::to_string(token_int);
    return token;
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

