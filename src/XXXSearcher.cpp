#include "Log.h"
#include "Config.h"
#include "XXXSearcher.h"

#define FIELDS_LEN 5

XXXSearcher::XXXSearcher()
{
    client = sphinx_create ( SPH_TRUE );
    sphinx_set_server ( client, Config::Instance()->sphinx_host_.c_str(), Config::Instance()->sphinx_port_ );
    sphinx_open ( client );

    sphinx_set_match_mode(client, SPH_MATCH_EXTENDED2);
    sphinx_set_ranking_mode(client, SPH_RANK_SPH04, NULL);
    sphinx_set_sort_mode(client, SPH_SORT_RELEVANCE, NULL);
    sphinx_set_limits(client, 0, 800, 800, 800);

    const char * field_names[FIELDS_LEN] =  {"title", "description", "keywords", "phrases", "exactly_phrases"};
    int field_weights[FIELDS_LEN] =  {50, 30, 70, 90 ,100};

    sphinx_set_field_weights( client, FIELDS_LEN, field_names, field_weights);
}

XXXSearcher::~XXXSearcher()
{
    sphinx_close ( client );
    sphinx_destroy ( client );
}

void XXXSearcher::makeFilter(const std::set<std::string>& keywords_guid)
{
    const char * attr_filter = "fguid";
    sphinx_int64_t *filter = new sphinx_int64_t[(int)keywords_guid.size()];
    int counts = 0;

    for(std::set<std::string>::const_iterator it = keywords_guid.begin(); it != keywords_guid.end(); ++it, counts++)
    {
        boost::crc_32_type  result;
        result.process_bytes((*it).data(), (*it).length());
        filter[counts] = result.checksum();
    }
    sphinx_add_filter( client, attr_filter, (int)keywords_guid.size(), filter, SPH_FALSE);
    delete [] filter;
}

void XXXSearcher::processKeywords(
    const std::vector<sphinxRequests> *sr,
    const std::set<std::string>& keywords_guid,
    Offer::Map &items)
{
    int i, tt, weight;
    //float startRating;
    float oldRating;
    std::string guid, match;
    std::map<std::string, float> mapOldGuidRating;

    try
    {
        sphinx_result * res;
        //Создаем фильтр
        makeFilter(keywords_guid);

        //Создаем запросы
        for (auto it = sr->begin(); it != sr->end(); ++it)
        {
            sphinx_add_query( client, (*it).query.c_str(), "worker", NULL );
        }

        res = sphinx_run_queries(client);
        if ( !res )
        {
            sphinx_reset_filters ( client );
            return;
        }
        for (tt=0; tt<sphinx_get_num_results(client); tt++)
        {
            for ( i=0; i<res->num_matches; i++ )
            {
                if (res->num_attrs == 8)
                {
                    guid = sphinx_get_string( res, i, 5 );
                    long id = strtol(guid.c_str(),NULL,10);
                    Offer::it p;
                    p = items.find(id);
                    if( p == items.end() )
                    {
                        continue;
                    }

                    Offer *pOffer = p->second;
                    /*
                    std::map<std::string, float>::iterator Ir = mapOldGuidRating.find(guid);
                    if ( Ir != mapOldGuidRating.end())
                    {
                        startRating = Ir->second;
                        oldRating = (*it)->rating;
                    }
                    else
                    {
                        startRating = oldRating = I->second.first.first;
                        mapOldGuidRating.insert(std::pair<std::string,float>(guid,startRating));
                    }
                    */
                    oldRating = pOffer->rating;
                    weight = sphinx_get_weight ( res, i );
                    pOffer->rating = weight * (*sr)[tt].rate;// * startRating;
                    if (pOffer->rating > oldRating)
                    {
//                            isOnClick = I->second.first.second.second.second;
//                            type = I->second.first.second.second.first;
                        match = (std::string) sphinx_get_string( res, i, 6 );

                        if(!pOffer->setBranch((*sr)[tt].branches))
                        {
                            Log::warn("Результат лишний: %s", guid.c_str());
                        }

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

                    }
                }
            }
        }
        res++;
        //LOG(INFO) << "Изменения массива предложений заняло: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";
        sphinx_reset_filters ( client );
    }
    catch (std::exception const &ex)
    {
        Log::warn("Непонятная sphinx ошибка: %s: %s", typeid(ex).name(), ex.what());
    }
    //LOG(INFO) << "Выход из обработки";

    return ;
}
