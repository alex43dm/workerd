#ifndef URLPARSER_H
#define URLPARSER_H

#include <map>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
using namespace std;

/** \brief Парсит URL */
// TODO: Поддержка UrlEncoded значений параметров
class UrlParser
{
public:
    /** \brief Парсит строку url */
    UrlParser(const string &url);

    /** \brief Возвращает карту параметров (key => value) */
    map<string, string> params() const
    {
        return params_;
    }

    /** \brief Возвращает параметр par или пустую строку, если параметр не найден */
    string param(const string &par) const;

private:
    string url_;
    map<string, string> params_;

    void parse();
    string percent_decode(const string &str) const;
    bool is_hex_digit(char hex_digit) const;
    int hex_digit_to_int(char hex_digit) const;


};




#endif // URLPARSER_H
