#pragma once
#include <string>
#include <list>
#include <set>
#include <map>

#include <stdlib.h>
#include <boost/crc.hpp>
#define _UNICODE
#include <sphinxclient.h>
#include <stdarg.h>
#include <stdlib.h>
#include <typeinfo>

#include "Offer.h"

class sphinxRequests
{
public:
    std::string query;
    float rate;
    std::string branches;
};

class XXXSearcher
{
public:
	XXXSearcher();
	~XXXSearcher();

	/** \brief Метод обработки запроса к индексу.
     *
	 */
    void processKeywords(const std::vector<sphinxRequests> *sr,
                         const std::set<std::string> &keywords_guid,
                         Offer::Map &items);
protected:
private:
    sphinx_client* client;

    void makeFilter(const std::set<std::string>& keywords_guid);
};
