#include "sphinxRequests.h"

sphinxRequests::sphinxRequests()
{
    //ctor
}

sphinxRequests::sphinxRequests(const std::string &q, float r, const EBranchT b):
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

std::string sphinxRequests::getBranchName() const
{
    switch(branches)
    {
        case EBranchT::T1:  return "user search";
        case EBranchT::T2: return "context";
        case EBranchT::T3: return "short term history";
        case EBranchT::T4: return "short long history";
        case EBranchT::T5: return "retargeting";
        default : return "";
    }
}
