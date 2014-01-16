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
#include "sphinxRequests.h"

class XXXSearcher
{
public:
	XXXSearcher();
	~XXXSearcher();

	/** \brief Метод обработки запроса к индексу.
     *
	 */
    void processKeywords(const std::vector<sphinxRequests> &sr, Offer::Map &items);
    void makeFilter(Offer::Map &items);

protected:
private:
    sphinx_client* client;
    bool makeFilterOn;
};
