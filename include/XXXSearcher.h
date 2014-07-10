#ifndef XXXSEARCHER_H
#define XXXSEARCHER_H

#include <vector>

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
    bool makeFilterOn;
    sphinx_int64_t *filter;
    std::vector<sphinxRequests> stringQuery;

    void dumpResult(sphinx_result *res) const;
};

#endif // XXXSEARCHER_H
