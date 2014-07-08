#ifndef XXXSEARCHER_H
#define XXXSEARCHER_H

#include <vector>

//#define _UNICODE
#include <sphinxclient.h>

#include "Offer.h"
#include "Informer.h"
#include "sphinxRequests.h"

class XXXSearcher
{
public:
    XXXSearcher();
    ~XXXSearcher();

    /** \brief Метод обработки запроса к индексу.
     *
     */
    void processKeywords(const std::vector<sphinxRequests> &sr, Offer::Map &items, float teasersMaxRating);

protected:
private:
    sphinx_client* client;
    void dumpResult(sphinx_result *res) const;
};

#endif // XXXSEARCHER_H
