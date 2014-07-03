#ifndef GEO_H
#define GEO_H

#include <string>

class Geo
{
    public:
        Geo(char *, size_t);
        virtual ~Geo();

        std::string compute(const std::string &country, const std::string &region);
        const char *str() const { return geo.c_str(); };

    protected:
    private:
        std::string geo;
        char *cmd;
        size_t len;
};

#endif // GEO_H
