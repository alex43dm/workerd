#ifndef PARAMPARSE_H
#define PARAMPARSE_H

#include <string>

#include <boost/regex/icu.hpp>

class ParamParse
{
    public:
        ParamParse();
        virtual ~ParamParse();

        std::string getKeywordsString(const std::string&);
        std::string getContextKeywordsString(const std::string&);

    protected:
    private:
        boost::u32regex replaceSymbol,replaceExtraSpace,replaceNumber;
        std::string stringWrapper(const std::string &str, bool replaceNumbers = false);
};

#endif // PARAMPARSE_H
