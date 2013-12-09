#pragma once
#include <string>
#include <list>
#include <vector>
#include <map>
#include <stdlib.h>
#include <boost/crc.hpp>
#include <CLucene.h>
//#include <CLucene/util/Misc.h>
//#include <CLucene/config/repl_wchar.h>
#include <CLucene/search/QueryFilter.h>
//#include <CLucene/_clucene-config.h>
#define _UNICODE
#include <sphinxclient.h>
#include <stdarg.h>
#include <stdlib.h>
#include <typeinfo>

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

using namespace std;
using namespace lucene::analysis;
using namespace lucene::index;
using namespace lucene::document;
using namespace lucene::queryParser;
using namespace lucene::search;
using namespace lucene::store;
using namespace lucene::util;

/** \brief Класс, обрабатывающий запросы к индексу. */
class XXXSearcher
{
public:
    XXXSearcher(const string& dirNameOffer, const string& dirNameInformer,
                const string& sphinx_hostname, int& sphinx_port);
    ~XXXSearcher();

    void setIndexParams(string &folder_offer, string &folder_informer,
                        string &sphinx_hostname, int &sphinx_port);


    /** \brief Метод обработки запроса к индексу.
     *
     */
    void processOffer(const string& qpart, const string& campaingsString, const string& categoryString, const string& filter, map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> &MAPSEARCH, map<string,string>& retargetingGuid);
    void processRating(const string& informerId, map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> &MAPSEARCH, set <string> &keywords_guid);
    void processKeywords(const list <pair<string,pair<float,string>>>& stringQuery, const set<string>& keywords_guid,  map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> &MAPSEARCH);

protected:
    string index_folder_offer_;
    string index_folder_informer_;
    Directory* dir;
    sphinx_client* client;

private:

};
