#include "Log.h"
#include "Config.h"
#include "XXXSearcher.h"


XXXSearcher::XXXSearcher() :
    indexName(Config::Instance()->sphinx_index_)
{
    client = sphinx_create ( SPH_TRUE );
    sphinx_set_server ( client, Config::Instance()->sphinx_host_.c_str(), Config::Instance()->sphinx_port_ );
    sphinx_open ( client );

    sphinx_set_match_mode(client,map_match[Config::Instance()->shpinx_match_mode_]);
    sphinx_set_ranking_mode(client, map_rank[Config::Instance()->shpinx_rank_mode_], NULL);
    sphinx_set_sort_mode(client, map_sort[Config::Instance()->shpinx_sort_mode_], NULL);
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
    delete [] filter;
    makeFilterOn = false;
}

//select 1 as doc, count(*) from worker group by doc;
void XXXSearcher::processKeywords(
    const std::vector<sphinxRequests> &sr,
    Offer::Map &items)
{
#ifdef DEBUG
    int counts;
        counts = 0;
#endif // DEBUG
    float oldRating;

    try
    {
        sphinx_result * res;

        //Создаем запросы
        for (auto it = sr.begin(); it != sr.end(); ++it)
        {
            sphinx_add_query( client, (*it).query.c_str(), indexName.c_str(), NULL );
#ifdef DEBUG
            std::clog<<__func__<<": "<<"query #"<<counts++<<" : "<<(*it).query<<std::endl;
#endif // DEBUG
        }

#ifdef DEBUG
            std::clog<<std::endl;
#endif // DEBUG

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
                dumpResult(res);
            }

            for ( int i=0; i<res->num_matches; i++ )
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
                    + (sr.size()>(unsigned)tt ? sr[tt].rate : 1) * maxRating
                    + weight;
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

                std::string match =  sphinx_get_string(res, i, 2);
                if ( match == "nomatch")
                {
                    pOffer->matching = (std::string)sphinx_get_string( res, i, 0 ) + " | " + (std::string)sphinx_get_string( res, i, 1 );
                }
                else if (match == "broadmatch")
                {
                    pOffer->matching = sphinx_get_string( res, i, 2 );
                }
                else if (match == "phrasematch")
                {
                    pOffer->matching = sphinx_get_string( res, i, 4 );
                }
                else if (match == "exactmatch")
                {
                    pOffer->matching = sphinx_get_string( res, i, 3 );
                }
                else
                {
                    std::clog<<"Результат: "<<pOffer->id_int<<" лишний"<<std::endl;
                    break;
                }

                if(cfg->logSphinx)
                {
                    std::clog<<"offer id: "<<pOffer->id_int
                    <<" old rating: "<<oldRating
                    <<" new: "<< pOffer->rating
                    <<" weight: "<< weight
                    <<" match by "<<match <<" what: "<<pOffer->matching
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

    std::clog<<"sphinx: num_matches:"<<res->num_matches<<", rating line: "<<midleRange<<std::endl;
    std::clog<<"sphinx: retrieved: "<< res->total<<" of matches: "<<res->total_found<<std::endl;

    for (i=0; i<res->num_words; i++ )
        std::clog<<"sphinx: query stats: "<<res->words[i].word<<" found "<<res->words[i].hits<<" times in "<<res->words[i].docs<<" documents"<<std::endl;

    for( i=0; i<res->num_matches; i++ )
    {
        std::clog<<"sphinx:  matches:#"<<1+i<<" doc_id="<<(int)sphinx_get_id ( res, i )<<", weight="<<sphinx_get_weight ( res, i )<<std::endl;

        for( j=0; j<res->num_attrs; j++ )
        {
            std::clog<<"sphinx:  matches: "<<res->attr_names[j]<<"=";
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
            std::clog<<std::endl;
        }
    }
}
