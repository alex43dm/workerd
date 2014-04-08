#include "Informer.h"

Informer::Informer(long id) :
    id(id)
{
}

Informer::Informer(long id, int capacity, const std::string &bannersCss,
                   const std::string &teasersCss, long domainId, long accountId)://, int rtgPercentage) :
    id(id),
    capacity(capacity),
    bannersCss(bannersCss),
    teasersCss(teasersCss),
    domainId(domainId),
    accountId(accountId),
//    rtgPercentage(rtgPercentage),
    RetargetingCount(0)
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
