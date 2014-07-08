#include <boost/algorithm/string.hpp>

#include "ParamParse.h"
#include "Log.h"
#include "Config.h"

ParamParse::ParamParse()
{
    replaceSymbol = boost::make_u32regex("[^а-яА-Яa-zA-Z0-9-]");
    replaceExtraSpace = boost::make_u32regex("\\s+");
    replaceNumber = boost::make_u32regex("(\\b)\\d+(\\b)");
}

ParamParse::~ParamParse()
{
    //dtor
}

/**
      \brief Нормализирует строку строку.
  */
std::string ParamParse::stringWrapper(const std::string &str, bool replaceNumbers)
{
    std::string t = str;
    //Заменяю все не буквы, не цифры, не минус на пробел
    t = boost::u32regex_replace(t,replaceSymbol," ");
    if (replaceNumbers)
    {
        //Заменяю отдельностояшие цифры на пробел, тоесть "у 32 п" замениться на
        //"у    п", а "АТ-23" останеться как "АТ-23"
        t = boost::u32regex_replace(t,replaceNumber," ");
    }
    //Заменяю дублируюшие пробелы на один пробел
    t = boost::u32regex_replace(t,replaceExtraSpace," ");
    boost::trim(t);
    return t;
}

std::string ParamParse::getKeywordsString(const std::string& str)
{
    try
    {
        std::string q = str;
        boost::algorithm::trim(q);
        if (q.empty())
        {
            return std::string();
        }
        std::string qs  = stringWrapper(q, false);
        std::string qsn = stringWrapper(q, true);

        std::vector<std::string> strs;
        std::string exactly_phrases;
        std::string keywords;
        boost::split(strs,qs,boost::is_any_of("\t "),boost::token_compress_on);
        for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it)
        {
            exactly_phrases += "<<" + *it + " ";
            if (it != strs.begin())
            {
                keywords += " | " + *it;
            }
            else
            {
                keywords += " " + *it;
            }
        }
        std::string str = "@exactly_phrases \"" + exactly_phrases + "\"~1 | @title \"" + qsn + "\"/3| @description \"" + qsn + "\"/3 | @keywords " + keywords + " | @phrases \"" + qs + "\"~5";
        return str;
    }
    catch (std::exception const &ex)
    {
        Log::err("exception %s: %s", typeid(ex).name(), ex.what());
        return std::string();
    }
}
std::string ParamParse::getContextKeywordsString(const std::string& query)
{
    try
    {
        std::string q, ret;

        q = query;
        boost::trim(q);
        if (q.empty())
        {
            return std::string();
        }
        std::string qs = stringWrapper(q);
        std::string qsn = stringWrapper(q, true);
        std::vector<std::string> strs;
        boost::split(strs,qs,boost::is_any_of("\t "),boost::token_compress_on);
        for(int i=0; i<Config::Instance()->sphinx_field_len_; i++)
        {
            std::string col = std::string(Config::Instance()->sphinx_field_names_[i]);
            std::string iret;
            for (std::vector<std::string>::iterator it = strs.begin(); it != strs.end(); ++it)
            {
                    if (it != strs.begin())
                    {
                        iret += " | @"+col+" "+*it;
                    }
                    else
                    {
                        iret += "@"+col+" "+*it;
                    }
                //exactly_phrases += "<<" + *it + " ";
            }
            if(i)
            {
                ret += "| "+iret+" ";
            }
            else
            {
                ret += " "+iret+" ";
            }
        }
/*
        return "((@exactly_phrases \"" + exactly_phrases + "\") \
        | (@title \"" + keywords + "\") \
        | (@description \"" + keywords + "\") \
        | (@keywords \"" + keywords + "\") \
        | (@phrases \"" + keywords + "\"))";*/
        return ret;

    }
    catch (std::exception const &ex)
    {
        Log::err("exception %s: %s", typeid(ex).name(), ex.what());
        return std::string();
    }
}
