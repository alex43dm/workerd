#ifndef SPHINXREQUESTS_H
#define SPHINXREQUESTS_H

#include <string>

class sphinxRequests
{
    public:
        std::string query;
        float rate;
        std::string branches;

        sphinxRequests();
        sphinxRequests(const std::string &q, float r, const std::string &b);
        virtual ~sphinxRequests();
    protected:
    private:
};

#endif // SPHINXREQUESTS_H
