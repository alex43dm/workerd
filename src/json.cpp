#include "json.h"

std::string Json::Utils::Escape(const std::string &str)
{
    std::string result;
    for (auto it = str.begin(); it != str.end(); it++)
    {
        switch (*it)
        {
        case '\t':
            result.append("\\t");
            break;
        case '"':
            result.append("\\\"");
            break;
        case '\\':
            result.append(" ");
            break;
        case '\'':
            result.append("\\'");
            break;
        case '/':
            result.append("\\/");
            break;
        case '\b':
            result.append("\\b");
            break;
        case '\r':
            result.append("\\r");
            break;
        case '\n':
            result.append("\\n");
            break;
        default:
            result.append(it, it + 1);
            break;
        }
    }
    return result;
}
