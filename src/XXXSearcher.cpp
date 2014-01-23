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
    const char * attr_filter = "guidint";
    sphinx_int64_t *filter = new sphinx_int64_t[(int)items.size()];
    int counts = 0;
//"index worker-full: no such filter attribute 'guid_int'"
    for(auto it = items.begin(); it != items.end(); ++it, counts++)
    {
        filter[counts] = (*it).second->id_int;
    }
    sphinx_add_filter( client, attr_filter, (int)items.size(), filter, SPH_FALSE);//SPH_MATCH_PHRASE
    delete [] filter;
    makeFilterOn = true;
}

void XXXSearcher::processKeywords(
    const std::vector<sphinxRequests> &sr,
    Offer::Map &items,
    Offer::Vector &result)
{
    int weight;
    //float startRating;
    float oldRating;
    std::string guid, match;
    std::map<std::string, float> mapOldGuidRating;
    Offer::MapRate resultImpression, resultClick, resultCategory, resultRetargeting, resultContext;

    try
    {
        sphinx_result * res;
        //Создаем фильтр
        if(!makeFilterOn)
        {
            makeFilter(items);
        }

        //Создаем запросы
        for (auto it = sr.begin(); it != sr.end(); ++it)
        {
            sphinx_add_query( client, (*it).query.c_str(), "worker-full", NULL );
        }

        res = sphinx_run_queries(client);
        if ( !res )
        {
            Log::warn("unligal sphinx result: %s", sphinx_error(client));
            goto default_return;
        }
        //process sphinx results
        int numRes = sphinx_get_num_results(client);
        for (int tt=0; tt < numRes; tt++, res++)
        {
            for ( int i=0; i<res->num_matches; i++ )
            {
                Log::info("sphinx match");
                if (res->num_attrs != 15)
                {
                    Log::warn("num_attrs: %d",res->num_attrs);
                    continue;
                }
                guid = sphinx_get_string( res, i, 13 );
                long id = strtol(guid.c_str(),NULL,10);
                Log::info("id: %s", guid.c_str());

                Offer::it p;
                p = items.find(id);
                if( p == items.end() )
                {
                    Log::warn("not found in items: %s", guid.c_str());
                    continue;
                }

                Offer *pOffer = p->second;

                if((int)sr.size() > tt && !pOffer->setBranch(sr[tt].branches))
                {
                    Log::warn("not found branch: %s", guid.c_str());
                }

                oldRating = pOffer->rating;
                weight = sphinx_get_weight ( res, i );
                pOffer->rating = weight * (int)sr.size() > tt ? sr[tt].rate : 1;// * startRating;

                if (pOffer->rating <= oldRating)
                {
                    continue;
                }

                match = (std::string) sphinx_get_string( res, i, 6 );

                if (match == "nomatch")
                {
                    pOffer->matching = (std::string) sphinx_get_string( res, i, 0 ) + " | " + (std::string) sphinx_get_string( res, i, 1 );
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
                    Log::warn("Результат лишний: %s", guid.c_str());
                    break;
                }

                if(pOffer->type == Offer::Type::banner)
                {
                    resultImpression.insert(Offer::Pair(pOffer->rating, pOffer));
                }
                else
                {
                    switch(pOffer->branch)
                    {
                    case EBranchL::L30:
                        resultClick.insert(Offer::Pair(pOffer->rating, pOffer));
                        break;
                    case EBranchL::L29:
                        resultCategory.insert(Offer::Pair(pOffer->rating, pOffer));
                        break;
                    case EBranchL::L28:
                        resultRetargeting.insert(Offer::Pair(pOffer->rating, pOffer));
                        break;
                    default:
                        resultContext.insert(Offer::Pair(pOffer->rating, pOffer));
                        break;
                    }
                }
            }
        }
    }
    catch (std::exception const &ex)
    {
        Log::warn("Непонятная sphinx ошибка: %s: %s: %s", typeid(ex).name(), ex.what(), sphinx_error(client));
    }
    //LOG(INFO) << "Выход из обработки";

    for(auto i = resultImpression.begin(); i != resultImpression.end(); ++i)
        result.push_back((*i).second);

    for(auto i = resultRetargeting.begin(); i != resultRetargeting.end(); ++i)
        result.push_back((*i).second);

    for(auto i = resultCategory.begin(); i != resultCategory.end(); ++i)
        result.push_back((*i).second);

    for(auto i = resultContext.begin(); i != resultContext.end(); ++i)
        result.push_back((*i).second);

    for(auto i = resultClick.begin(); i != resultClick.end(); ++i)
        result.push_back((*i).second);

default_return:
    //copy back map to result vector
    if(result.size() < items.size())
    {
        for(auto i = items.begin(); i != items.end(); ++i)
            result.push_back((*i).second);
    }

    sphinx_reset_filters ( client );
    makeFilterOn = false;

    return;
}
