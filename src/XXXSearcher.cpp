#include "Log.h"
#include "Config.h"
#include "XXXSearcher.h"

#define FIELDS_LEN 6

XXXSearcher::XXXSearcher()
{
    client = sphinx_create ( SPH_TRUE );
    sphinx_set_server ( client, Config::Instance()->sphinx_host_.c_str(), Config::Instance()->sphinx_port_ );
    sphinx_open ( client );

    sphinx_set_match_mode(client, SPH_MATCH_EXTENDED2);
    sphinx_set_ranking_mode(client, SPH_RANK_SPH04, NULL);
    sphinx_set_sort_mode(client, SPH_SORT_RELEVANCE, NULL);
    sphinx_set_limits(client, 0, 800, 800, 800);

    const char * field_names[FIELDS_LEN] =  {"title", "description", "keywords", "exactly_phrases", "phrases", "minuswords"};
    int field_weights[FIELDS_LEN] =  {50, 30, 70, 100, 90, 100};

    sphinx_set_field_weights( client, FIELDS_LEN, field_names, field_weights);

    makeFilterOn = false;
}

XXXSearcher::~XXXSearcher()
{
    sphinx_close ( client );
    sphinx_destroy ( client );
}

void XXXSearcher::makeFilter(Offer::Map &items)
{
    makeFilterOn = true;
}

//select 1 as doc, count(*) from worker group by doc;
void XXXSearcher::processKeywords(
    const std::vector<sphinxRequests> &sr,
    Offer::Map &items,
    Offer::Vector &result)
{
    float midleRange = 0;
    int counts;
//    int weight;
    //float startRating;
    float oldRating;
//    std::string guid, match;
//    std::map<std::string, float> mapOldGuidRating;
    sphinx_int64_t *filter;

    try
    {
        sphinx_result * res;

        sphinx_set_select(client,"guidint,rating");

        //Создаем фильтр
        filter = (sphinx_int64_t *)new sphinx_int64_t[(int)items.size()];
        counts = 0;
        for(Offer::it it = items.begin(); it != items.end(); ++it)
        {
            filter[counts++] = (*it).first;
            midleRange += (*it).second->rating;
        }

        midleRange /= counts;

        if(sphinx_add_filter( client, "guidint", counts, filter, SPH_FALSE)!=SPH_TRUE)
        {
            Log::warn("sphinx filter is not working: %s", sphinx_error(client));
        }

        //Создаем запросы
        counts = 0;
        for (auto it = sr.begin(); it != sr.end(); ++it)
        {
            sphinx_add_query( client, (*it).query.c_str(), "worker-full", NULL );
            #ifdef DEBUG
            printf("query%d: %s\n",counts++,(*it).query.c_str());
            #endif // DEBUG
        }

        res = sphinx_run_queries(client);
        if(!res)
        {
            Log::warn("unligal sphinx result: %s", sphinx_error(client));
            goto default_return;
        }
#ifdef DEBUG
        int i,j, k, mva_len;;
        unsigned int * mva;

        printf("retrieved %d of %d matches\n", res->total, res->total_found );

        printf ( "Query stats:\n" );
        for (i=0; i<res->num_words; i++ )
            printf ( "\t'%s' found %d times in %d documents\n",
            res->words[i].word, res->words[i].hits, res->words[i].docs );

        printf ( "\nMatches:\n" );
	for ( i=0; i<res->num_matches; i++ )
	{
		printf ( "%d. doc_id=%d, weight=%d", 1+i,
			(int)sphinx_get_id ( res, i ), sphinx_get_weight ( res, i ) );

		for ( j=0; j<res->num_attrs; j++ )
		{
			printf ( ", %s=", res->attr_names[j] );
			switch ( res->attr_types[j] )
			{
			case SPH_ATTR_MULTI64:
			case SPH_ATTR_MULTI:
				mva = sphinx_get_mva ( res, i, j );
				mva_len = *mva++;
				printf ( "(" );
				for ( k=0; k<mva_len; k++ )
					printf ( k ? ",%u" : "%u", ( res->attr_types[j]==SPH_ATTR_MULTI ? mva[k] : (unsigned int)sphinx_get_mva64_value ( mva, k ) ) );
				printf ( ")" );
				break;

			case SPH_ATTR_FLOAT:	printf ( "%f", sphinx_get_float ( res, i, j ) ); break;
			case SPH_ATTR_STRING:	printf ( "%s", sphinx_get_string ( res, i, j ) ); break;
			default:				printf ( "%u", (unsigned int)sphinx_get_int ( res, i, j ) ); break;
			}
		}

		printf ( "\n" );
	}
	printf ( "\n" );

#endif // DEBUG
        //process sphinx results
        int numRes = sphinx_get_num_results(client);
        for (int tt=0; tt < numRes; tt++, res++)
        {
            if (res->status == SEARCHD_ERROR)
            {
                Log::warn("sphinx: %s",res->error);
                continue;
            }

            if(res->status == SEARCHD_WARNING)
            {
                Log::warn("sphinx: %s",res->warning);
            }

            if (res->num_matches > 0)
            {
                Log::info("sphinx match, rating line: %f",midleRange);
            }

            for ( int i=0; i<res->num_matches; i++ )
            {
                if (res->num_attrs != 2)
                {
                    Log::warn("num_attrs: %d",res->num_attrs);
                    continue;
                }

                long long id = sphinx_get_int(res, i, 0);
//                Log::info("id: %lld", id);

                Offer::it p;
                p = items.find(id);
                if( p == items.end() )
                {
                    Log::warn("not found in items: %lld", id);
                    continue;
                }

                Offer *pOffer = p->second;
                //weight = sphinx_get_weight (res, i );
                oldRating = pOffer->rating;
                pOffer->rating = pOffer->rating + Config::Instance()->range_context_ * midleRange + sphinx_get_float(res, i, 1);
                Log::gdb("id: %lld old rating: %f new: %f", pOffer->id_int, oldRating, pOffer->rating);
                //pOffer->rating = weight * (int)sr.size() > tt ? sr[tt].rate : 1;// * startRating;
            }
        }
    }
    catch (std::exception const &ex)
    {
        Log::warn("Непонятная sphinx ошибка: %s: %s: %s", typeid(ex).name(), ex.what(), sphinx_error(client));
    }

default_return:
    //copy back map to result vector
Offer::MapRate resultAll;
        for(auto i = items.begin(); i != items.end(); ++i)
            resultAll.insert(Offer::PairRate((*i).second->rating, (*i).second));

        for(auto i = resultAll.begin(); i != resultAll.end(); ++i)
            result.push_back((*i).second);


    for(auto i = items.begin(); i != items.end(); ++i)
        result.push_back((*i).second);

    sphinx_reset_filters ( client );
    delete [] filter;
//    makeFilterOn = false;

    return;
}
