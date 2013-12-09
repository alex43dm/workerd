#include"UserHistory.h"
/** id_user_ - идентификатор пользователя. сейчас идентификатором выступает ip адрес */
UserHistory::UserHistory(const string& id_user_)
{
    id_user = id_user_;
}

UserHistory::~UserHistory()
{
    id_user.clear();
    shortHistory.clear();
    longHistory.clear();
    deprecatedOffers.clear();
    contextHistory.clear();
    categoryHistory.clear();
    retargetingHistory.clear();
}

void UserHistory::addShortHistory(const string& keyWord)
{
    shortHistory.push_back(keyWord);
}

void UserHistory::addLongHistory(const string& keyWord)
{
    longHistory.push_back(keyWord);
}

void UserHistory::addDeprecatedOffer(const string& offerId)
{
    deprecatedOffers.push_back(offerId);
}
void UserHistory::addContextHistory(const string& keyWord)
{
    contextHistory.push_back(keyWord);

}
void UserHistory::addCategory(const string& catId)
{
    categoryHistory.push_back(catId);
}
void UserHistory::addRetargeting(const string& redOfferId)
{
    retargetingHistory.push_back(redOfferId);
}

string UserHistory::historyToLog()
{
    string str;
    list<string>::iterator it;
    str.append("UserHistory in detail....\n");
    str.append("ip: ");
    str.append(id_user);
    str.append("\n");
    str.append("short history:\n");
    for ( it=getShortHistory().begin() ; it != getShortHistory().end(); it++ )
    {
        str.append(*it);
        str.append("\n");
    }
    str.append("long  history:\n");
    for ( it=getLongHistory().begin() ; it != getLongHistory().end(); it++ )
    {
        str.append(*it);
        str.append("\n");
    }
    str.append("deprecated history:\n");
    for ( it=getDeprecatedOffers().begin() ; it != getDeprecatedOffers().end(); it++ )
    {
        str.append(*it);
        str.append("\n");
    }
    return str;
}
std::string UserHistory::UserHistoryToJson(std::string query)
{
    std::stringstream json;
    list<string>::iterator it;

    json << "{";
    json << "\"search\": \"" << query << "\",";
    json << "\"ShortTermHistory\": {";
    for (it=getShortHistory().begin() ; it != getShortHistory().end(); it++ )
    {
        if (it != getShortHistory().begin())
            json << ",";
        json <<"\"" << (*it) << "\"" ;
    }
    json << "},";
    json << "\"LongTermHistory\": {";
    for (it=getLongHistory().begin() ; it != getLongHistory().end(); it++ )
    {
        if (it != getLongHistory().begin())
            json << ",";
        json <<"\"" << (*it) << "\"" ;
    }
    json << "}";
    json << "}";
    return json.str();
}
