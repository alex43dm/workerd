#pragma once
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include "XXXSearcher.h"
#include "../Campaign.h"
#include "../UserHistory.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include "../Log.h"
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>

using namespace std;
using namespace boost::algorithm;

/** \brief Функция, сравнивающая пары a и b по возрастанию веса. Возвращает a.second < b.second. */
bool rigthOrderPairCompare(const pair< pair<string, float>, pair<string, pair<string, string>> >  &a, const pair< pair<string, float>, pair<string, pair<string, string>> >  &b);
/** \brief Функция, сравнивающая пары a и b по убыванию веса. Возвращает a.second > b.second. */
bool inverseOrderPairCompare(const pair< pair<string, float>, pair<string, pair<string, string>> >  &a, const pair< pair<string, float>, pair<string, pair<string, string>> >  &b);

/** \brief Структура, хранящая веса для всех шести запросов к lucene */
struct Weights
{
public:
    float range_priority_banners;///< запрос 1 по "крутым" баннерам
    float range_query;///< запрос 2 по query
    float range_context;///< запрос 3 по контексту страницы
    float range_shot_term;///< запрос 4 по shortHistory
    float range_context_term;///< запрос 5 по контексту страницы
    float range_long_term;///< запрос 6 по longHistory
    float range_on_places;///< запрос 7 - РП по размещению

    Weights()
    {
        range_priority_banners = 1.0;
        range_query = 1.0;
        range_shot_term = 1.0;
        range_long_term = 1.0;
        range_context = 1.0;
        range_context_term = 1.0;
        range_on_places = 1.0;
    }
};

/** \brief Класс-обёртка работы с CLucene и Sphinx.
 * Построение индекса производится вне модуля. Модуль лишь читает индекс.
 * Индекс располагается на жёстком диске.
 * Индекс хранит документы. В качестве документов выступают рекламные предложения из основной базы данных,
 * с которой работает модуль. В индекс занесены (проиндексированы) следующие поля:
 * \code
 * guid - идентификатор РП.
 * informer_ids - идентификатор информера, за которым закреплено РП.
 * campaign_id - идентификатор кампании, к которой принадлежит данное РП.
 * type - тип РП.
 * \endcode
 *
 */
class SearcherWrapper
{
protected:
    XXXSearcher *searcher;  ///< исполнитель запросов к индексу.
    string dirNameOffer;    ///< имя папки, в которой расположен индекс. Вбито в код: "/var/www/index".
    string dirNameInformer;
    string index_folder_informer;
    string sphinx_hostname;
    int sphinx_port;
public:
    SearcherWrapper()
    {

        dirNameOffer = "/var/www/indexOffer";
        dirNameInformer = "/var/www/indexInformer";
        sphinx_hostname = "localhost";
        sphinx_port = 9312;
        searcher = new XXXSearcher(dirNameOffer, dirNameInformer, sphinx_hostname, sphinx_port);
    }
    ~SearcherWrapper()
    {
        delete searcher;
    }


    void setIndexParams(string &folder_offer, string &folder_informer)
    {
        dirNameOffer = folder_offer;
        dirNameInformer = folder_informer;
        sphinx_hostname = "localhost";
        sphinx_port = 9312;
        searcher-> setIndexParams(dirNameOffer, dirNameInformer, sphinx_hostname, sphinx_port);
    }

protected:

    /** \brief Перевод содержимого списка в строку через пробел.
     @param l Список строк.
     @return Строку, сотоящую из строк, содержащихся в списке, перечисленных через пробел.

     Пример:
     \code
     Список l:
     <str1>
     <str2>
     <str3>

     Выход метода: "str1 str2 str3"
     \endcode

     */
    string listOfStringsToString(const list<string>& l)
    {
        string str;
        list<string>::const_iterator p;
        for (p=l.begin(); p!=l.end(); p++)
        {
            if (!p->empty())
            {
                str += (*p + " ");
            }
        }
        return str;
    }


    /**
        \brief Нормализирует строку строку.

    */
    string stringWrapper(const string &str, bool replaceNumbers = false)
    {
        boost::u32regex replaceSymbol = boost::make_u32regex("[^а-яА-Яa-zA-Z0-9-]");
        boost::u32regex replaceExtraSpace = boost::make_u32regex("\\s+");
        boost::u32regex replaceNumber = boost::make_u32regex("(\\b)\\d+(\\b)");
        string t = str;
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

    /** \brief Метод получения списка кампаний в "готовом" виде.

     Т.е. чтоб строку можно было просто вставить в запрос к индексу без дополнительных действий.

     @param campaigns Список кампаний, отобранных для показа.
     @return Строку, содержащую список кампаний campaigns, готовую для непосредственной вставки в запрос к индексу.

     Пример:
     \code
     Список campaigns:
     <кампания1>
     <кампания2>
     <кампания3>

     Выход метода: "+campaign_id:(кампания1 кампания2 кампания3)"
     return "+campaign_id:(6aa5383e-5ddb-4eb5-b693-d96acd17409b 6fcec5d5-3976-4ae0-a0dd-11056762af80 bd15f42a-643b-4d64-b24b-9b14d6b02095)";
     \endcode
     */
    string getCampaignsString(const list<Campaign>& campaigns)
    {
        // +campaign_id:(все значения из списка кампаний)
        if (campaigns.empty())
        {
            return string();
        }
        string str = "+campaign_id:(";
        list<Campaign>::const_iterator p;

        for (p=campaigns.begin(); p!=campaigns.end(); p++)
        {
            str += (p->id()+ " ");

        }
        str += ") ";
        return str;
    }


    string constructFilter(const string &deprecatedString)
    {
        string qstr = "+type:(teaser banner) ";
        if(deprecatedString.empty() )
        {
            Log::info("deprecated history empty");
            return "";
        }
        else
        {
            qstr += " -(" + deprecatedString + ") ";
        }
        qstr.push_back(0);

        return qstr;
    }
    string constructCategory(const list<string>& categoryString)
    {

        if (categoryString.empty())
        {
            return string();
        }
        string catstr;
        list<string>::const_iterator p;
        for (p=categoryString.begin(); p!=categoryString.end(); p++)
        {
            if (!p->empty())
            {
                if (p != categoryString.begin())
                {
                    catstr += (" OR " + *p);
                }
                else
                {
                    catstr += (" " + *p);
                }
            }
        }
        if (catstr.empty())
        {
            return string();
        }
        string qstr = "+category_ids:(" + catstr + ")";
        return qstr;
    }
    /* *
     * Конструктор запроса для выбора РП по ключевым словам
     * */
    string getKeywordsString(const string& query)
    {
        try
        {
            string q = query;
            boost::trim(q);
            if (q.empty())
            {
                return string();
            }
            string qs = stringWrapper(q);
            string qsn = stringWrapper(q, true);
            vector<string> strs;
            string exactly_phrases;
            string keywords;
            boost::split(strs,qs,boost::is_any_of("\t "),boost::token_compress_on);
            for (vector<string>::iterator it = strs.begin(); it != strs.end(); ++it)
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
            string str = "((@exactly_phrases \"" + exactly_phrases + "\"~1) | (@title \"" + qsn + "\"/3)| (@description \"" + qsn + "\"/3) | (@keywords " + keywords + " ) | (@phrases \"" + qs + "\"~5))";
            return str;
        }
        catch (std::exception const &ex)
        {
            Log::err("exception %s : %s query: %s", typeid(ex).name(), ex.what(), query.c_str());
            return string();
        }
    }

    /* *
     * Конструктор запроса для выбора РП по ключевым словам из контекста
     * */
    string getContextKeywordsString(const string& query)
    {
        try
        {
            string q = query;
            boost::trim(q);
            if (q.empty())
            {
                return string();
            }
            string qs = stringWrapper(q);
            string qsn = stringWrapper(q, true);
            vector<string> strs;
            string exactly_phrases;
            string keywords;
            boost::split(strs,qs,boost::is_any_of("\t "),boost::token_compress_on);
            for (vector<string>::iterator it = strs.begin(); it != strs.end(); ++it)
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
            string str = "((@exactly_phrases \"" + exactly_phrases + "\"~1) | (@title \"" + qsn + "\"/3)| (@description \"" + qsn + "\"/3) | (@keywords \"" + qs + "\"/3 ) | (@phrases \"" + qs + "\"~5))";
            return str;
        }
        catch (std::exception const &ex)
        {
            Log::err("exception %s : %s query: %s", typeid(ex).name(), ex.what(), query.c_str());
            return string();
        }
    }
    /**
     * Конструктор запроса для выбора РП на места.
     * +informer_ids:(ALL наш_информер)
     * +type:teaser
    */
    string getOnPlacesString(const string& informerId)
    {
        if (informerId.empty())
        {
            return string();
        }
        string str = "+informer_ids:(all OR " + informerId + ")";
        return str;
    }


public:
    /**
     * Обращение к индексу CLucene с коэффициентами в структуре Weights.
     * Этот метод не производит непосредственное обращение, он только формирует
     * необходимый запрос и отдаёт его на выполнению классу XXXSearcher.
     */
    list< pair< pair<string, float>, pair<string, pair<string, string>> > > f1(const string& query, UserHistory& userHistory, const string& context, const string& informerId, const list<Campaign>& campaigns, const list<Campaign>& campaignsSoc, const list<Campaign>& campaignsAllGeo, const Weights &w)
    {
//		uint64_t strr = Misc::currentTimeMillis();
        //общие части всех запросов - список кампаний и начальное множество deprecatedOffers
        map<string,string> retargetingGuid;
        list<string>::const_iterator retit;
        for (retit=userHistory.getRetargetingHistory().begin(); retit!=userHistory.getRetargetingHistory().end(); retit++)
        {
            string retGuidOffer = *retit;
            if (!retGuidOffer.empty())
            {
                retargetingGuid.insert(pair<string,string>(retGuidOffer, retGuidOffer));
            }
        }

        // +campaign_id:(все значения из списка кампаний выбраных для показа с учетом привязки к РБ и гео-временных таргетинговых настроек)
        string strCamp = getCampaignsString(campaigns);
        string strSocCamp = getCampaignsString(campaignsSoc);

        // +campaign_id:(все значения из списка кампаний выбраных для показа с без учета привязки к РБ но с учетом
        // гео-временных таргетинговых настроек)
        string strCampAllGeo = getCampaignsString(campaignsAllGeo);

        //начальное множество deprecatedOffers - просто список идентификаторов РП неподлежаших к показу,
        //но в виде строки, чтоб не формировать при каждом запросе.
        string deprStr;
        deprStr = listOfStringsToString(userHistory.getDeprecatedOffers());
        deprStr = constructFilter(deprStr);

        //множество categoryOffers - просто список идентификаторов,
        //но в виде строки, чтоб не формировать при каждом запросе.
        string categoryStr;
        categoryStr = constructCategory(userHistory.getCategoryHistory());

        string q;
        list <pair<string,pair<float,string>>> stringQuery;
        ////Выбор подходящих РП.
        //
        //
        ////
        map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> MAPSEARCH;
        if(!campaigns.empty())
        {
            q = getOnPlacesString(informerId);
            searcher->processOffer(q, strCamp, categoryStr, deprStr, MAPSEARCH, retargetingGuid);
            Log::info("Выбрано processOffer РП: %d ", MAPSEARCH.size());
//            VLOG(1) << "Выборка processOffer заняла: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";
        }
        else
        {
            Log::warn("Список платных кампаний пуст");
        }
        ////Выбор социальных РП.
        //
        //
        ////
        map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> SOCMAPSEARCH;
        if(!campaignsSoc.empty())
        {
            q = getOnPlacesString(informerId);
            searcher->processOffer(q, strSocCamp, categoryStr, deprStr, SOCMAPSEARCH, retargetingGuid);
            Log::info("Выбрано soc processOffer РП %d", SOCMAPSEARCH.size());
//            VLOG(1) << "Выборка soc processOffer заняла: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";
        }
        else
        {
            Log::warn("Список социальных кампаний пуст");
        }
        ////Обнавление рейтингов РП и создание списка GUID.
        //
        //
        ////
        set <string> keywords_guid;
        searcher->processRating(informerId, MAPSEARCH, keywords_guid);
//		VLOG(1) << "Обновдение рейтинга заняло: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";

        ////Создание запросов выбор РП по ключевым словам.
        //
        //
        ////

        string strSS = query;
        if (!strSS.empty())
        {
            Log::info("Запрос по П/З query = %s", strSS.c_str());
            q = getKeywordsString(strSS);
            if (!q.empty())
            {
                stringQuery.push_back(pair<string,pair<float,string>>(q, pair<float,string>(w.range_query,"T1")));
            }
        }

        string strCS = context;
        if (!strCS.empty())
        {
            Log::info("Запрос по контексту страницы context = %s", strCS.c_str());
            q = getContextKeywordsString(strCS);
            if (!q.empty())
            {
                stringQuery.push_back(pair<string,pair<float,string>>(q, pair<float,string>(0.1,"T2")));
            }
        }

        list<string>::const_iterator i;
        for (i=userHistory.getShortHistory().begin(); i!=userHistory.getShortHistory().end(); i++)
        {
            string strSH = *i;
            if (!strSH.empty())
            {
                Log::info("Запрос по К.И query = %s", strSH.c_str());
                q = getKeywordsString(strSH);
                if (!q.empty())
                {
                    stringQuery.push_back(pair<string,pair<float,string>>(q, pair<float,string>(w.range_shot_term,"T3")));
                }
            }
        }

        list<string>::const_iterator itrc;
        for (itrc=userHistory.getContextHistory().begin(); itrc!=userHistory.getContextHistory().end(); itrc++)
        {
            string strCH = *itrc;
            if (!strCH.empty())
            {
                Log::info("Запрос по Контекст Истории = %s", strCH.c_str());
                q = getContextKeywordsString(strCH);
                if (!q.empty())
                {
                    stringQuery.push_back(pair<string,pair<float,string>>(q, pair<float,string>(0.1,"T4")));
                }
            }
        }

        list<string>::const_iterator itr;
        for (itr=userHistory.getLongHistory().begin(); itr!=userHistory.getLongHistory().end(); itr++)
        {
            string strLH = *itr;
            if (!strLH.empty())
            {
                Log::info("Запрос по Д.И = %s", strLH.c_str());
                q = getKeywordsString(strLH);
                if (!q.empty())
                {
                    stringQuery.push_back(pair<string,pair<float,string>>(q, pair<float,string>(w.range_long_term,"T5")));
                }
            }
        }
//		VLOG(1) << "Построение запросов заняла: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";
        ////Обнавление РП по релевантности запроса и формируем результат.
        //
        //
        ////
        searcher->processKeywords(stringQuery, keywords_guid, MAPSEARCH);
//		VLOG(1) << "Выборка заняла: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";

        ////сортируем и формируем результат
        //
        //
        ////
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> resultImpression;
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> resultRetargeting;
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> resultContext;
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> resultCategory;
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> resultClick;
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> resultSoc;
        list<pair<pair<string, float>, pair<string, pair<string, string>>>> result;
        map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>::const_iterator p = MAPSEARCH.begin();
        while(p!=MAPSEARCH.end())
        {
            if (p->second.second.second.first == "false")
            {
                if (p->second.first.second.first.second == "false")
                {
                    if(p->second.first.second.second.first == "banner" and p->second.first.second.second.second == "false")
                    {
                        resultImpression.push_back(pair<pair<string, float>,pair<string, pair<string, string>>>(
                                                       (pair<string, float>(p->first,p->second.first.first)),
                                                       (pair<string, pair<string, string>>(p->second.first.second.first.first,(pair<string, string>(p->second.second.first.first,p->second.second.first.second))))));
                    }
                    else
                    {
                        if(p->second.first.second.first.first == "L30")
                        {
                            resultClick.push_back(pair<pair<string, float>,pair<string, pair<string, string>>>(
                                                      (pair<string, float>(p->first,p->second.first.first)),
                                                      (pair<string, pair<string, string>>(p->second.first.second.first.first,(pair<string, string>(p->second.second.first.first,p->second.second.first.second))))));
                        }
                        else if(p->second.first.second.first.first == "L29")
                        {
                            resultCategory.push_back(pair<pair<string, float>,pair<string, pair<string, string>>>(
                                                         (pair<string, float>(p->first,p->second.first.first)),
                                                         (pair<string, pair<string, string>>(p->second.first.second.first.first,(pair<string, string>(p->second.second.first.first,p->second.second.first.second))))));
                        }
                        else if(p->second.first.second.first.first == "L28")
                        {
                            resultRetargeting.push_back(pair<pair<string, float>,pair<string, pair<string, string>>>(
                                                            (pair<string, float>(p->first,p->second.first.first)),
                                                            (pair<string, pair<string, string>>(p->second.first.second.first.first,(pair<string, string>(p->second.second.first.first,p->second.second.first.second))))));
                        }
                        else
                        {
                            resultContext.push_back(pair<pair<string, float>,pair<string, pair<string, string>>>(
                                                        (pair<string, float>(p->first,p->second.first.first)),
                                                        (pair<string, pair<string, string>>(p->second.first.second.first.first,(pair<string, string>(p->second.second.first.first,p->second.second.first.second))))));
                        }
                    }
                }
            }
            p++;
        }
        map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>::const_iterator ps = SOCMAPSEARCH.begin();
        while(ps!=SOCMAPSEARCH.end())
        {
            resultSoc.push_back(pair<pair<string, float>,pair<string, pair<string, string>>>(
                                    (pair<string, float>(ps->first,ps->second.first.first)),
                                    (pair<string, pair<string, string>>(ps->second.first.second.first.first,(pair<string, string>(ps->second.second.first.first,ps->second.second.first.second))))));
            ps++;
        }

        resultImpression.sort(inverseOrderPairCompare);
        Log::info("Банеров %d", resultImpression.size());
        resultRetargeting.sort(inverseOrderPairCompare);
        Log::info("Ретаргетинга %d", resultRetargeting.size());
        resultClick.sort(inverseOrderPairCompare);
        Log::info("Мест размешения %d", resultClick.size());
        resultContext.sort(inverseOrderPairCompare);
        resultCategory.sort(inverseOrderPairCompare);
        result.insert(result.end(), resultImpression.begin(), resultImpression.end());
        result.insert(result.end(), resultRetargeting.begin(), resultRetargeting.end());
        result.insert(result.end(), resultCategory.begin(), resultCategory.end());
        result.insert(result.end(), resultContext.begin(), resultContext.end());
        result.insert(result.end(), resultClick.begin(), resultClick.end());
        result.insert(result.end(), resultSoc.begin(), resultSoc.end());
        Log::info("Всего выбрано РП %d", result.size());
//		uint32_t srch = (int32_t)(Misc::currentTimeMillis() - strr);
//		VLOG(1) << "Всего выборка РП заняла: " << srch << " мс";
        return result;
    }
};
