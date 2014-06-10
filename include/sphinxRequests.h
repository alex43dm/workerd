#ifndef SPHINXREQUESTS_H
#define SPHINXREQUESTS_H

#include <string>
#include "EBranch.h"

class sphinxRequests
{
    public:
        std::string query;
        float rate;
        EBranchT branches;

        sphinxRequests();
        sphinxRequests(const std::string &q, float r, const EBranchT b);
        virtual ~sphinxRequests();
        std::string getBranchName() const;
    protected:
    private:
};

#endif // SPHINXREQUESTS_H
