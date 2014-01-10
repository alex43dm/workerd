#include <fstream>
/*
#include <cstring>
#include <iostream>
*/
#include "SearchEngines.h"

bool SearchEngineMapContainer::setSearchEnginesMap(const std::string& filename)
{
    std::string str;
    std::string name;
    std::string::iterator it;
    std::string::size_type i;
    std::vector<std::string> v;

    std::ifstream input (filename);
    if(input.fail())
    {
        return false;
    }
    while(getline(input,str))
    {

        if(!str.empty())
        {
            //удалить пробелы из строки
            it = str.begin();
            while(it!=str.end())
            {
                if(*it == ' ')
                {
                    it = str.erase(it);
                }
                else
                {
                    it++;
                }
            }
            //найти name
            i = str.find(":");
            name = str.substr(0, i);
            str = str.substr(i+1);

            //выделять значения параметров
            i = str.find(",");
            while(true)
            {
                if(i==std::string::npos)
                {
                    if(!str.empty())
                        v.push_back(str);
                    break;
                }
                else
                {
                    v.push_back(str.substr(0,i));
                    str = str.substr(i+1);
                    i = str.find(",");
                }
            }

            m_listSSearchEngines[name] = v;
            v.clear();

        }
    }
    input.close();
    return true;
}
