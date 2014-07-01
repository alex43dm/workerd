#include "Informer.h"

Informer::Informer(long id) :
    id(id)
{
}

Informer::Informer(long id, int capacity, const std::string &bannersCss,
                   const std::string &teasersCss, long domainId, long accountId,
                   double range_short_term, double range_long_term,
                   double range_context, double range_search, int retargeting_capacity, bool blocked):
    id(id),
    capacity(capacity),
    bannersCss(bannersCss),
    teasersCss(teasersCss),
    domainId(domainId),
    accountId(accountId),
    retargeting_capacity(retargeting_capacity),
    range_short_term(range_short_term),
    range_long_term(range_long_term),
    range_context(range_context),
    range_search(range_search),
    blocked(blocked)
{
}

Informer::~Informer()
{
}

bool Informer::operator==(const Informer &other) const
{
    return this->id == other.id;
}

bool Informer::operator<(const Informer &other) const
{
    return this->id < other.id;
}
