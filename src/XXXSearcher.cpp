#include <map>
#include <string>

#include <typeinfo>

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
    sphinx_close ( client );
    sphinx_destroy ( client );
}

void XXXSearcher::makeFilter(Offer::Map &items)
{
    if(makeFilterOn)
        return;

    sphinx_set_select(client,"guidint,rating,match,sphrases,skeywords,sexactly_phrases");

    //Создаем фильтр
    filter = (sphinx_int64_t *)new sphinx_int64_t[(int)items.size()];
    int counts = 0;
    midleRange = 0;
    maxRating = 0;

    for(Offer::it it = items.begin(); it != items.end(); ++it)
    {
        filter[counts++] = (*it).first;
        midleRange += (*it).second->rating;
        //max
        if(maxRating < (*it).second->rating)
            maxRating = (*it).second->rating;
    }

    midleRange /= counts;

    if(sphinx_add_filter( client, "guidint", counts, filter, SPH_FALSE)!=SPH_TRUE)
    {
        Log::warn("sphinx filter is not working: %s", sphinx_error(client));
    }
    makeFilterOn = true;
}

void XXXSearcher::cleanFilter()
{
    sphinx_reset_filters ( client );
    if(filter)
        delete [] filter;
    makeFilterOn = false;
}

//select 1 as doc, count(*) from worker group by doc;
void XXXSearcher::processKeywords(
    const std::vector<sphinxRequests> &sr,
    Offer::Map &items)
{
    float oldRating;


    if( sr.size() == 0 )
    {
        if(cfg->logSphinx)
        {
            std::clog<<"sphinx: 0 request"<<std::endl;
        }
        return;
    }

    try
    {
        sphinx_result * res;

        //Создаем запросы
        for (auto it = sr.begin(); it != sr.end(); ++it)
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
                std::clog<<"sphinx: request #"<<tt<<" by: "<<sr[tt].getBranchName()<<" query: "<<sr[tt].query<<std::endl;

                dumpResult(res);
            }

            for( int i=0; i<res->num_matches; i++ )
            {
                if (res->num_attrs != 6)
                {
                    std::clog<<"num_attrs: "<<res->num_attrs<<std::endl;
                    continue;
                }

                long long id = sphinx_get_int(res, i, 0);

                Offer::it p;
                p = items.find(id);
                if( p == items.end() )
                {
                    std::clog<<__func__<<": not found in items: "<<id<<std::endl;
                    continue;
                }

                Offer *pOffer = p->second;

                float weight = sphinx_get_weight (res, i ) / 1000;

                oldRating = pOffer->rating;
                pOffer->rating = pOffer->rating
                    + (sr.size()>(unsigned)tt ? sr[tt].rate : 1)
                    * (maxRating + weight);
                    //+ sphinx_get_float(res, i, 1);

                //pOffer->rating = weight * (int)sr.size() > tt ? sr[tt].rate : 1;// * startRating;

                //user search update
                /*
                if(sr.size()>(unsigned)tt && sr[tt].branches == EBranchT::T1)//user search
                {
                    const char * attr = "rating";
                    sphinx_uint64_t * docids;
                    sphinx_int64_t * values;

                    sphinx_update_attributes(client, indexName.c_str(),
                                             1,
                                             attr,
                                             int num_docs,
                                             const sphinx_uint64_t * docids,
                                             const sphinx_int64_t * values );
                }*/

                pOffer->matching =  sphinx_get_string(res, i, 2);

                pOffer->setBranch(sr[tt].branches);

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
    <<" rating line: "<<midleRange
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
