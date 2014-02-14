#pragma once

#include <string>
#include <vector>
//#include <list>
//#include <set>
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
private:
    std::map<std::string,int> map_match =
    {
        { "SPH_MATCH_ALL", SPH_MATCH_ALL },
        { "SPH_MATCH_ANY", SPH_MATCH_ANY },
        { "SPH_MATCH_PHRASE", SPH_MATCH_PHRASE },
        { "SPH_MATCH_BOOLEAN", SPH_MATCH_BOOLEAN },
        { "SPH_MATCH_EXTENDED", SPH_MATCH_EXTENDED },
        { "SPH_MATCH_FULLSCAN", SPH_MATCH_FULLSCAN },
        { "SPH_MATCH_EXTENDED2", SPH_MATCH_EXTENDED2 }
    };

    std::map<std::string,int> map_rank =
    {
        { "SPH_RANK_PROXIMITY_BM25", SPH_RANK_PROXIMITY_BM25 },
        { "SPH_RANK_BM25", SPH_RANK_BM25 },
        { "SPH_RANK_NONE", SPH_RANK_NONE },
        { "SPH_RANK_WORDCOUNT", SPH_RANK_WORDCOUNT },
        { "SPH_RANK_PROXIMITY", SPH_RANK_PROXIMITY },
        { "SPH_RANK_MATCHANY", SPH_RANK_MATCHANY },
        { "SPH_RANK_FIELDMASK", SPH_RANK_FIELDMASK },
        { "SPH_RANK_SPH04", SPH_RANK_SPH04 },
        { "SPH_RANK_EXPR", SPH_RANK_EXPR },
        { "SPH_RANK_TOTAL", SPH_RANK_TOTAL }
    };
    std::map<std::string,int> map_sort =
    {
        { "SPH_SORT_RELEVANCE", SPH_SORT_RELEVANCE },
        { "SPH_SORT_ATTR_DESC", SPH_SORT_ATTR_DESC },
        { "SPH_SORT_ATTR_ASC", SPH_SORT_ATTR_ASC },
        { "SPH_SORT_TIME_SEGMENTS", SPH_SORT_TIME_SEGMENTS },
        { "SPH_SORT_EXTENDED", SPH_SORT_EXTENDED },
        { "SPH_SORT_EXPR", SPH_SORT_EXPR }
    };

public:
	XXXSearcher();
	~XXXSearcher();

	/** \brief Метод обработки запроса к индексу.
     *
	 */
    void processKeywords(const std::vector<sphinxRequests> &sr, Offer::Map &items);
    void makeFilter(Offer::Map &items);
    void cleanFilter();

protected:
private:
    std::string indexName;
    sphinx_client* client;
    bool makeFilterOn;
    sphinx_int64_t *filter;
    float midleRange, maxRating;

    void dumpResult(sphinx_result *res) const;
};
