#include <map>
#include <string>

#include <typeinfo>

#include <boost/algorithm/string.hpp>

#include "Log.h"
#include "Config.h"
#include "XXXSearcher.h"

std::map<std::string,int> map_sph_match =
{
    { "SPH_MATCH_ALL", SPH_MATCH_ALL },
    { "SPH_MATCH_ANY", SPH_MATCH_ANY },
    { "SPH_MATCH_PHRASE", SPH_MATCH_PHRASE },
    { "SPH_MATCH_BOOLEAN", SPH_MATCH_BOOLEAN },
    { "SPH_MATCH_EXTENDED", SPH_MATCH_EXTENDED },
    { "SPH_MATCH_FULLSCAN", SPH_MATCH_FULLSCAN },
    { "SPH_MATCH_EXTENDED2", SPH_MATCH_EXTENDED2 }
};

std::map<std::string,int> map_sph_rank =
{
    { "SPH_RANK_PROXIMITY_BM25", SPH_RANK_PROXIMITY_BM25 },
    { "SPH_RANK_BM25", SPH_RANK_BM25 },
    { "SPH_RANK_NONE", SPH_RANK_NONE },
    { "SPH_RANK_WORDCOUNT", SPH_RANK_WORDCOUNT },
    { "SPH_RANK_PROXIMITY", SPH_RANK_PROXIMITY },
    { "SPH_RANK_MATCHANY", SPH_RANK_MATCHANY },
    { "SPH_RANK_FIELDMASK", SPH_RANK_FIELDMASK },
    { "SPH_RANK_SPH04", SPH_RANK_SPH04 },
    { "SPH_RANK_EXPR", SPH_RANK_EXPR },
    { "SPH_RANK_TOTAL", SPH_RANK_TOTAL }
};

std::map<std::string,int> map_sph_sort =
{
    { "SPH_SORT_RELEVANCE", SPH_SORT_RELEVANCE },
    { "SPH_SORT_ATTR_DESC", SPH_SORT_ATTR_DESC },
    { "SPH_SORT_ATTR_ASC", SPH_SORT_ATTR_ASC },
    { "SPH_SORT_TIME_SEGMENTS", SPH_SORT_TIME_SEGMENTS },
    { "SPH_SORT_EXTENDED", SPH_SORT_EXTENDED },
    { "SPH_SORT_EXPR", SPH_SORT_EXPR }
};

XXXSearcher::XXXSearcher()
{
    replaceSymbol = boost::make_u32regex("[^а-яА-Яa-zA-Z0-9-]");
    replaceExtraSpace = boost::make_u32regex("\\s+");
    replaceNumber = boost::make_u32regex("(\\b)\\d+(\\b)");

    m_pPrivate = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init((pthread_mutex_t*)m_pPrivate, &attr);
    pthread_mutexattr_destroy(&attr);

    client = sphinx_create ( SPH_TRUE );
    sphinx_set_server ( client, Config::Instance()->sphinx_host_.c_str(), Config::Instance()->sphinx_port_ );
    sphinx_open ( client );

    sphinx_set_match_mode(client,map_sph_match[Config::Instance()->shpinx_match_mode_]);
    sphinx_set_ranking_mode(client, map_sph_rank[Config::Instance()->shpinx_rank_mode_], NULL);
    sphinx_set_sort_mode(client, map_sph_sort[Config::Instance()->shpinx_sort_mode_], NULL);
    sphinx_set_limits(client, 0, 800, 800, 800);

    sphinx_set_field_weights( client,
                             Config::Instance()->sphinx_field_len_,
                             Config::Instance()->sphinx_field_names_,
                             Config::Instance()->sphinx_field_weights_);
    makeFilterOn = false;
}

XXXSearcher::~XXXSearcher()
{
    pthread_mutex_destroy((pthread_mutex_t*)m_pPrivate);

    sphinx_close ( client );
    sphinx_destroy ( client );
}

//select 1 as doc, count(*) from worker group by doc;
void XXXSearcher::processKeywords(
    Offer::Map &items,
    float teasersMaxRating)
{
    float oldRating;


    if( stringQuery.size() == 0 )
    {
        if(cfg->logSphinx)
        {
            std::clog<<"sphinx: 0 request"<<std::endl;
        }
        return;
    }

    try
    {

        makeFilter(items);

        sphinx_result * res;

        //Создаем запросы
        for (auto it = stringQuery.begin(); it != stringQuery.end(); ++it)
        {
            sphinx_add_query( client, (*it).query.c_str(), cfg->sphinx_index_.c_str(), NULL );
        }

        res = sphinx_run_queries(client);
        if(!res)
        {
            std::clog<<__func__<<": unligal sphinx result: "<<sphinx_error(client)<<std::endl;
            return;
        }

        //process sphinx results
        int numRes = sphinx_get_num_results(client);
        for (int tt=0; tt < numRes; tt++, res++)
        {
            if (res->status == SEARCHD_ERROR)
            {
                std::clog<<__func__<<": SEARCHD_ERROR: "<<res->error<<std::endl;
                continue;
            }

            if(res->status == SEARCHD_WARNING)
            {
                std::clog<<__func__<<": SEARCHD_WARNING: "<<res->warning<<std::endl;
            }

            if(cfg->logSphinx)
            {
                std::clog<<"sphinx: request #"<<tt<<" by: "<<stringQuery[tt].getBranchName()<<" query: "<<stringQuery[tt].query<<std::endl;

                dumpResult(res);
            }

            for( int i=0; i<res->num_matches; i++ )
            {
                if (res->num_attrs < 1)
                {
                    std::clog<<"num_attrs: "<<res->num_attrs<<std::endl;
                    continue;
                }

                unsigned long long id = sphinx_get_int(res, i, 0);

                if(items.count(id) == 0)
                {
                    std::clog<<__func__<<": not found in items: "<<id<<std::endl;
                    continue;
                }

                Offer *pOffer = items[id];

                float weight = sphinx_get_weight (res, i ) / 1000;

                oldRating = pOffer->rating;
                pOffer->rating = pOffer->rating
                    + (stringQuery.size()>(unsigned)tt ? stringQuery[tt].rate : 1)
                    * (teasersMaxRating + weight);
                    //+ sphinx_get_float(res, i, 1);

                for (int i=0; i<res->num_words; i++ )
                    pOffer->matching += " " + std::string(res->words[i].word);

                pOffer->setBranch(stringQuery[tt].branches);

                if(cfg->logSphinx)
                {
                    std::clog<<"sphinx: offer id: "<<pOffer->id_int
                    <<" old rating: "<<oldRating
                    <<" new: "<< pOffer->rating
                    <<" branch: "<<pOffer->getBranch()
                    <<std::endl;
                }
            }
        }
    }
    catch (std::exception const &ex)
    {
        std::clog<<"Непонятная sphinx ошибка: "<<typeid(ex).name()<<" "<<ex.what()<<" "<<sphinx_error(client);
    }

    return;
}

void XXXSearcher::dumpResult(sphinx_result *res) const
{
    int i,j, k, mva_len;;
    unsigned int * mva;

    std::clog<<"sphinx: total: "<< res->total
    <<" total_found: "<<res->total_found
    <<" num_matches: "<<res->num_matches
    <<std::endl;

    for (i=0; i<res->num_words; i++ )
        std::clog<<"sphinx: query stats: "<<res->words[i].word
        <<" found "<<res->words[i].hits
        <<" times in "<<res->words[i].docs<<" documents"<<std::endl;

    for( i=0; i<res->num_matches; i++ )
    {
        std::clog<<"sphinx:  matches:#"<<1+i
        <<" doc_id="<<(int)sphinx_get_id ( res, i )
        <<", weight="<<sphinx_get_weight ( res, i )
        <<" by: ";

        for( j=0; j<res->num_attrs; j++ )
        {
            if(res->attr_types[j] == SPH_ATTR_STRING)
            {
                std::string mstring = sphinx_get_string(res,i,j);
                if(!mstring.empty() && mstring.size()>1)
                {
                    std::clog<<" "<<res->attr_names[j]<<"="<<mstring;
                    continue;
                }
            }

            std::clog<<" "<<res->attr_names[j]<<"=";

            switch ( res->attr_types[j] )
            {
            case SPH_ATTR_MULTI64:
            case SPH_ATTR_MULTI:
                mva = sphinx_get_mva ( res, i, j );
                mva_len = *mva++;
                std::clog<< "(";
                for ( k=0; k<mva_len; k++ )
                    std::clog<<( res->attr_types[j]==SPH_ATTR_MULTI ? mva[k] : (unsigned int)sphinx_get_mva64_value ( mva, k ) );
                std::clog<<")";
                break;

            case SPH_ATTR_FLOAT:
                std::clog<<sphinx_get_float ( res, i, j );
                break;
            case SPH_ATTR_STRING:
                std::clog<<sphinx_get_string ( res, i, j );
                break;
            default:
                std::clog<<(unsigned int)sphinx_get_int ( res, i, j );
                break;
            }

            std::clog<<" ";
        }

        std::clog<<std::endl;
    }
}

void XXXSearcher::makeFilter(Offer::Map &items)
{
    if(makeFilterOn)
        return;

    sphinx_set_select(client,"fguid");

    //Создаем фильтр
    filter = (sphinx_int64_t *)new sphinx_int64_t[(int)items.size()];
    int counts = 0;

    for(Offer::it it = items.begin(); it != items.end(); ++it)
    {
        filter[counts++] = (*it).second->id_int;
    }

    if(sphinx_add_filter( client, "fguid", counts, filter, SPH_FALSE)!=SPH_TRUE)
    {
        Log::warn("sphinx filter is not working: %s", sphinx_error(client));
    }

    makeFilterOn = true;
}

void XXXSearcher::cleanFilter()
{
    if(makeFilterOn)
    {
        sphinx_reset_filters ( client );
        if(filter)
            delete [] filter;
        makeFilterOn = false;
    }

    stringQuery.clear();
}



/**
      \brief Нормализирует строку строку.
  */
std::string XXXSearcher::stringWrapper(const std::string &str, bool replaceNumbers)
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

std::string XXXSearcher::getKeywordsString(const std::string& str)
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

std::string XXXSearcher::getContextKeywordsString(const std::string& query)
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

        return ret;

    }
    catch (std::exception const &ex)
    {
        Log::err("exception %s: %s", typeid(ex).name(), ex.what());
        return std::string();
    }
}

void XXXSearcher::addRequest(const std::string req, float rate, const EBranchT br)
{
    if(req.empty())
    {
        return;
    }

    std::string q = req;
    boost::trim(q);
    if(q.empty())
    {
        return;
    }

    std::vector<std::string> vStr;
    std::string res, col;

    boost::split(vStr,q,boost::is_any_of("\t "),boost::token_compress_on);

    for(int i=0; i<cfg->sphinx_field_len_; i++)
    {
        col = std::string(cfg->sphinx_field_names_[i]);

        for(auto p=vStr.begin(); p != vStr.end(); ++p)
        {
            if(p == vStr.begin())
            {
                res = col + " " + *p;
            }
            else
            {
                res += " |" + col + " " + *p;
            }
        }
    }

    pthread_mutex_lock((pthread_mutex_t*)m_pPrivate);
    stringQuery.push_back(sphinxRequests(req, rate, br));
    pthread_mutex_unlock((pthread_mutex_t*)m_pPrivate);
}
