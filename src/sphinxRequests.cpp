#include "sphinxRequests.h"

sphinxRequests::sphinxRequests()
{
    //ctor
}

sphinxRequests::sphinxRequests(const std::string &q, float r, const std::string &b):
    query(q),
    rate(r),
    branches(b)
{
    //ctor
}
sphinxRequests::~sphinxRequests()
{
    //dtor
}
