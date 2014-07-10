#ifndef XXXSEARCHER_H
#define XXXSEARCHER_H

#include <vector>
#include <boost/regex/icu.hpp>
#include <sphinxclient.h>

#include "Offer.h"
#include "Informer.h"
#include "sphinxRequests.h"

class XXXSearcher
{
public:
    XXXSearcher();
    ~XXXSearcher();

    void addRequest(const std::string, float,  const EBranchT);
    void processKeywords(Offer::Map &items, float teasersMaxRating);
    void makeFilter(Offer::Map &items);
    void cleanFilter();

protected:
private:
    pthread_mutex_t *m_pPrivate;
    sphinx_client* client;
    boost::u32regex replaceSymbol,replaceExtraSpace,replaceNumber;
    bool makeFilterOn;
    sphinx_int64_t *filter;
    std::vector<sphinxRequests> stringQuery;


    void dumpResult(sphinx_result *res) const;
    std::string stringWrapper(const std::string &str, bool replaceNumbers = false);
    std::string getKeywordsString(const std::string& str);
    std::string getContextKeywordsString(const std::string& query);
};

#endif // XXXSEARCHER_H
