#include "../Log.h"
#include "XXXSearcher.h"

std::string toString(const TCHAR *s)
{
    wstring ws(s);
    return string(ws.begin(), ws.end());
}

XXXSearcher::XXXSearcher(const string& dirNameOffer, const string& dirNameInformer,
                         const string& sphinx_hostname, int& sphinx_port)
{
    Log::info("Constructor XXXSearcher");
    index_folder_offer_ = dirNameOffer;
    index_folder_informer_ = dirNameInformer;
    client = sphinx_create ( SPH_TRUE );
    sphinx_set_server ( client, "localhost", sphinx_port );
    sphinx_open ( client );
}

XXXSearcher::~XXXSearcher()
{
    sphinx_close ( client );
    sphinx_destroy ( client );
    Log::info("Destructor XXXSearcher ");
}

void XXXSearcher::setIndexParams(string &folder_offer, string &folder_informer,
                                 string &sphinx_hostname, int &sphinx_port)
{
    index_folder_offer_ = folder_offer;
    index_folder_informer_ = folder_informer;
    sphinx_set_server ( client, "localhost", sphinx_port );
    Log::info("Set index params");
}

void XXXSearcher::processOffer(const string& qpart, const string& campaingsString, const string& categoryString, const string& filter,  map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> &MAPSEARCH, map<string,string>& retargetingGuid)
{
    if (qpart.empty())
    {
        //LOG(INFO) << "пустой запрос.";
        return;
    }
    string query;
    string queryCategory;
    string queryRetargeting;
    if (categoryString.empty())
    {
        query  = campaingsString + " "+qpart + " +retargeting:(false) ";
        query.push_back(0);
    }
    else
    {
        query  = campaingsString + " "+qpart + " +retargeting:(false) ";
        query.push_back(0);
        queryCategory  = campaingsString + " "+ qpart + " "+ categoryString + " +retargeting:(false) ";
        queryCategory.push_back(0);
    }
    bool findRetargiting = false;
    if (retargetingGuid.size() > 0)
    {
        findRetargiting = true;
        queryRetargeting =  campaingsString + " "+qpart + " +retargeting:(true) ";
        queryRetargeting += "+guid:( ";
        map <string,string>::iterator cur;
        for(cur=retargetingGuid.begin(); cur!=retargetingGuid.end(); cur++)
        {
            if (cur != retargetingGuid.begin())
            {
                queryRetargeting += (" OR " + (*cur).first);
            }
            else
            {
                queryRetargeting += (" " + (*cur).first);
            }
        }
        queryRetargeting += " )";
        queryRetargeting.push_back(0);

    }

    Log::info("query: %s queryCategory: %s queryRetargeting: %s", query.c_str(), queryCategory.c_str(), queryRetargeting.c_str());

    PerFieldAnalyzerWrapper *analyzer;
    analyzer = new PerFieldAnalyzerWrapper(new WhitespaceAnalyzer());

    IndexReader* reader = IndexReader::open(index_folder_offer_.c_str());
//=======================================================================================
    try
    {
        IndexReader* newreader = reader->reopen();
        if ( newreader != reader )
        {
            _CLLDELETE(reader);
            reader = newreader;
        }
        IndexSearcher s(reader);

//=======================================================================================
        QueryParser* qp = _CLNEW QueryParser(_T("guid"),analyzer);
        Query* q = qp->parse(std::wstring(query.begin(), query.end()).c_str());
        _CLDELETE(qp);
//---------------------------------------------------------------------------------------
        Query* qTemp = NULL;
        if (!filter.empty())
        {
//---------------------------------------------------------------------------------------
            QueryParser* qp1 = _CLNEW QueryParser(_T("guid"),analyzer);
            qTemp = qp1->parse(std::wstring(filter.begin(), filter.end()).c_str());
            _CLDELETE(qp1);
        }
//=======================================================================================
        Sort * _sort = NULL;
        _sort = _CLNEW Sort();
        _sort->setSort (_T("rating"), true);
        QueryFilter * f = NULL;
        if (!filter.empty())
            f = _CLNEW QueryFilter(qTemp);

        Hits* h = s.search(q, f, _sort);

        size_t k= h->length();
        if (k > 1000)
            k = 1000;
        Log::info("Выбрано РП Clucene: %d", k);

            Document* doc;
            float rating;
            string matching;
            string match;
            string branch;
            string type;
            string isOnClick;
            string contextOnly;
            string retargeting;
            string guid;
            for (size_t i=0; i<k; i++)
            {
            doc = &h->doc(i);
            wstring ws(doc->get(_T("retargeting")));
            retargeting = string(ws.begin(),ws.end());
            if (retargeting == "false" )
            {
                branch = "L30";
                match = "place";
                rating = 0;
                guid = toString(doc->get(_T("guid")));
                type = toString(doc->get(_T("type")));
                isOnClick = toString(doc->get(_T("isonclick")));
                contextOnly = toString(doc->get(_T("contextonly")));
                rating = atof(toString(doc->get(_T("rating"))).c_str());
                if ((isOnClick == "false") and (type == "banner"))
                {
                    branch = "L1";
                }
                matching.clear();
                MAPSEARCH.insert(
                        pair<string,pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>(
                            guid,
                            pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>(
                                pair<float,pair<pair<string,string>,pair<string,string>>>(
                                        rating,
                                        pair<pair<string,string>,pair<string,string>>(
                                        pair<string,string>(branch,contextOnly),
                                        pair<string,string>(type,isOnClick)
                                        )
                                    ),
                                    pair<pair<string,string>,pair<string,string>>(
                                        pair<string,string>(match,matching),
                                        pair<string,string>(retargeting,""))
                                )
                            )
                        );
            }
            doc->clear();
            }

        _CLLDELETE(h);
        _CLLDELETE(q);
        _CLLDELETE(f);
        _CLLDELETE(qTemp);
        _CLDELETE(_sort);
        s.close();
    }
    catch(CLuceneError &err)
    {
        Log::err("Ошибка при запросе к lucene: %s", err.what());
    }
    catch(...)
    {
        Log::err("Непонятная ошибка");
    }
//=======================================================================================
    if (findRetargiting)
    {
        try
        {
            IndexReader* newreader = reader->reopen();
            if ( newreader != reader )
            {
                _CLLDELETE(reader);
                reader = newreader;
            }
            IndexSearcher sR(reader);
            //=======================================================================================
            QueryParser* qpR = _CLNEW QueryParser(_T("guid"),analyzer);
            Query* qR = qpR->parse(std::wstring(queryRetargeting.begin(), queryRetargeting.end()).c_str());
            _CLDELETE(qpR);
            //---------------------------------------------------------------------------------------
            Query* qTempR = NULL;
            if (!filter.empty())
            {
                //---------------------------------------------------------------------------------------
                QueryParser* qp1R = _CLNEW QueryParser(_T("guid"),analyzer);
                qTempR = qp1R->parse(std::wstring(filter.begin(), filter.end()).c_str());
                _CLDELETE(qp1R);
            }
            //=======================================================================================
            Sort * _sortR = NULL;
            _sortR = _CLNEW Sort();
            _sortR->setSort (_T("rating"), true);
            QueryFilter * fR = NULL;
            if (!filter.empty())
                fR = _CLNEW QueryFilter(qTempR);

            Hits* hR = sR.search(qR, fR, _sortR);

            size_t kR= hR->length();
            if (kR > 100)
                kR = 100;
            Log::info("Выбрано ретаргетинговых РП Clucene %d", kR);

            Document* doc;
            float rating;
            string matching;
            string match;
            string branch;
            string type;
            string isOnClick;
            string contextOnly;
            string retargeting;
            string guid;
            for (size_t i=0; i<kR; i++)
            {
                doc = &hR->doc(i);
                type = toString(doc->get(_T("type")));
                isOnClick = toString(doc->get(_T("isonclick")));
                retargeting = toString(doc->get(_T("retargeting")));
                if ((retargeting == "true" ) and (type == "teaser") and (isOnClick == "true") )
                {
                    Log::info("Добавлен ретаргенинговый РП");
                    match = "place";
                    rating = 0;
                    guid = toString(doc->get(_T("guid")));
                    branch = "L28";
                    retargeting = "false";
                    rating = atof(toString(doc->get(_T("rating"))).c_str());
                    contextOnly = toString(doc->get(_T("contextonly")));
                    matching.clear();
                    MAPSEARCH.insert(
                            pair<string,pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>(
                                guid,
                                pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>(
                                    pair<float,pair<pair<string,string>,pair<string,string>>>(
                                            rating,
                                            pair<pair<string,string>,pair<string,string>>(
                                            pair<string,string>(branch,contextOnly),
                                            pair<string,string>(type,isOnClick)
                                            )
                                        ),
                                        pair<pair<string,string>,pair<string,string>>(
                                            pair<string,string>(match,matching),
                                            pair<string,string>(retargeting,""))
                                    )
                                )
                            );
                }
                doc->clear();
            }

            _CLLDELETE(hR);
            _CLLDELETE(qR);
            _CLLDELETE(fR);
            _CLLDELETE(qTempR);
            _CLDELETE(_sortR);
            sR.close();
        }
        catch(CLuceneError &err)
        {
            Log::err("Ошибка при запросе к lucene: %s", err.what());
        }
        catch(...)
        {
            Log::err("Непонятная ошибка");
        }
    }
//=======================================================================================
//=======================================================================================

//=======================================================================================
//=======================================================================================
    if(!queryCategory.empty())
    {
        try
        {
            IndexReader* newreader = reader->reopen();
            if ( newreader != reader )
            {
                _CLLDELETE(reader);
                reader = newreader;
            }
            IndexSearcher sC(reader);
            //=======================================================================================
            QueryParser* qpC = _CLNEW QueryParser(_T("guid"),analyzer);
            Query* qC = qpC->parse(std::wstring(queryCategory.begin(), queryCategory.end()).c_str());
            _CLDELETE(qpC);
            //---------------------------------------------------------------------------------------
            Query* qTempC = NULL;
            if (!filter.empty())
            {
                //---------------------------------------------------------------------------------------
                QueryParser* qp1C = _CLNEW QueryParser(_T("guid"),analyzer);
                qTempC = qp1C->parse(std::wstring(filter.begin(), filter.end()).c_str());
                _CLDELETE(qp1C);
            }
            //=======================================================================================
            QueryFilter * fC = NULL;
            if (!filter.empty())
                fC = _CLNEW QueryFilter(qTempC);

            Hits* hC = sC.search(qC, fC, NULL);

            size_t k= hC->length();
            Log::info("Обнавлено РП %d", k);
            Document* doc;
            string guid;
            string retargeting;
            for (size_t i=0; i<k; i++)
            {
                doc = &hC->doc(i);
                guid = toString(doc->get(_T("guid")));
                retargeting = toString(doc->get(_T("retargeting")));
                if (retargeting == "false" )
                {
                    map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>::iterator I = MAPSEARCH.find(guid);
                    if ( I != MAPSEARCH.end())
                    {
                        I->second.first.second.first.first = "L29";
                    }
                }
                doc->clear();
            }

            _CLLDELETE(hC);
            _CLLDELETE(qC);
            _CLLDELETE(fC);
            _CLLDELETE(qTempC);
            sC.close();
        }
        catch(CLuceneError &err)
        {
            Log::err("Ошибка при запросе к lucene: %s", err.what());
        }
        catch(...)
        {
            Log::err("Непонятная ошибка");
        }
    }
//=======================================================================================
//=======================================================================================

    reader->close();
    _CLLDELETE(reader);

    delete analyzer;

    return;
}



void XXXSearcher::processRating(const string& informerId, map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> &MAPSEARCH, set <string> &keywords_guid)
{

    map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>::const_iterator p = MAPSEARCH.begin();
    string mapKeysString;
    string id;
    while(p!=MAPSEARCH.end())
    {
        id = p->first;
        if (!id.empty())
        {
            mapKeysString += (id + " ");
            keywords_guid.insert(id);
        }
        p++;
    }
    if (mapKeysString.empty())
    {
        return;
    }
    string query =  "+offer:("+mapKeysString+") +informer:("+informerId+")";
    query.push_back(0);

    PerFieldAnalyzerWrapper *analyzer;
    analyzer = new PerFieldAnalyzerWrapper(new WhitespaceAnalyzer());
    analyzer->addAnalyzer(_T("offer"), new WhitespaceAnalyzer());
    analyzer->addAnalyzer(_T("informer"), new WhitespaceAnalyzer());

    IndexReader* reader = IndexReader::open(index_folder_informer_.c_str());
    try
    {

        IndexReader* newreader = reader->reopen();
        if ( newreader != reader )
        {
            _CLLDELETE(reader);
            reader = newreader;
        }
        IndexSearcher s(reader);

        QueryParser* qp = _CLNEW QueryParser(_T("offer"),analyzer);
        Query* q = qp->parse(std::wstring(query.begin(), query.end()).c_str());
        _CLDELETE(qp);
//============================================================================

        Hits* h = s.search(q);

        size_t k = h->length();
        //VLOG(2) << "Найдено = " << h->length();

        Document* doc;
        string guid;
        float rating;
        for (size_t i=0; i<k; i++)
        {
            rating = 0;
            doc = &h->doc(i);
 			guid = toString(doc->get(_T("offer")));
 			rating = atof(toString(doc->get(_T("rating"))).c_str());
            map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>::iterator I = MAPSEARCH.find(guid);
            if ( I != MAPSEARCH.end())
            {
                I->second.first.first = rating;
            }
            doc->clear();
        }


        _CLLDELETE(h);
        _CLLDELETE(q);
        s.close();

    }
    catch(CLuceneError &err)
    {
        Log::err("Ошибка при запросе к lucene: %s", err.what());
    }
    catch(...)
    {
        Log::err("Непонятная ошибка");
    }

    reader->close();
    _CLLDELETE(reader);
    delete analyzer;

    return;
}

void XXXSearcher::processKeywords(const list <pair<string,pair<float,string>>>& stringQuery, const set<string>& keywords_guid, map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>> &MAPSEARCH)
{

    int i, tt, weight, counts;
    float weight_rate, rating, startRating, oldRating;
    string guid, matching, match, branch, qbranch, isOnClick, type;
    map<string, float> mapOldGuidRating;
    try
    {
        sphinx_set_match_mode(client, SPH_MATCH_EXTENDED2);
        sphinx_set_ranking_mode(client, SPH_RANK_SPH04, NULL);
        sphinx_set_sort_mode(client, SPH_SORT_RELEVANCE, NULL);
        sphinx_set_limits(client, 0, 800, 800, 800);
        sphinx_result * res;
        const char * field_names[5];
        int field_weights[5];
        field_names[0] = "title";
        field_names[1] = "description";
        field_names[2] = "keywords";
        field_names[3] = "phrases";
        field_names[4] = "exactly_phrases";
        field_weights[0] = 50;
        field_weights[1] = 30;
        field_weights[2] = 70;
        field_weights[3] = 90;
        field_weights[4] = 100;
        sphinx_set_field_weights( client, 5, field_names, field_weights );
        field_weights[0] = 1;
        field_weights[1] = 1;
        field_weights[2] = 1;
        field_weights[3] = 1;
        field_weights[4] = 1;
        //Создаем фильтр
        const char * attr_filter = "fguid";
        sphinx_int64_t *filter = new sphinx_int64_t[(int)keywords_guid.size()];
        counts = 0;
        for(set<string>::const_iterator it = keywords_guid.begin(); it != keywords_guid.end(); it++)
        {
            boost::crc_32_type  result;
            result.process_bytes((*it).data(), (*it).length());
            filter[counts] = result.checksum();
            counts++;
        }
        sphinx_add_filter( client, attr_filter, (int)keywords_guid.size(), filter, SPH_FALSE);
        //Создаем запросы
        //uint64_t strr = Misc::currentTimeMillis();
        float *rate = new float[(int)stringQuery.size()];
        string *branches = new string[(int)stringQuery.size()];
        counts = 0;
        for (list <pair<string,pair<float,string>>>::const_iterator it = stringQuery.begin(); it != stringQuery.end(); ++it)
        {
            string query = it->first;
            rate[counts] = it->second.first;
            branches[counts] = it->second.second;
            const char* q = query.c_str ();
            //LOG(INFO) << q;
            sphinx_add_query( client, q, "worker", NULL );
            counts++;
        }
        //LOG(INFO) << "Создание запроса заняло: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";
        //strr = Misc::currentTimeMillis();
        res = sphinx_run_queries(client);
        if ( !res )
        {
            sphinx_reset_filters ( client );
            delete [] rate;
            delete [] branches;
            delete [] filter;
            return;
        }
        for (tt=0; tt<sphinx_get_num_results(client); tt++)
        {
            for ( i=0; i<res->num_matches; i++ )
            {
                if (res->num_attrs == 8)
                {
                    guid = sphinx_get_string( res, i, 5 );
                    map<string, pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>>::iterator I = MAPSEARCH.find(guid);
                    if ( I != MAPSEARCH.end())
                    {
                        map<string, float>::iterator Ir = mapOldGuidRating.find(guid);
                        if ( Ir != mapOldGuidRating.end())
                        {
                            startRating = Ir->second;
                            oldRating = I->second.first.first;
                        }
                        else
                        {
                            startRating = oldRating = I->second.first.first;
                            mapOldGuidRating.insert(pair<string,float>(guid,startRating));
                        }
                        weight = sphinx_get_weight ( res, i );
                        weight_rate = rate[tt];
                        rating = weight * weight_rate * startRating;
                        if (rating > oldRating)
                        {
                            qbranch = branches[tt];
                            isOnClick = I->second.first.second.second.second;
                            type = I->second.first.second.second.first;
                            match = (string) sphinx_get_string( res, i, 6 );
                            if (type == "banner" and isOnClick == "false" and qbranch == "T1")
                            {
                                branch = "L2";
                                rating = 1000 * rating;
                            }
                            else if (type == "banner" and isOnClick == "false" and qbranch == "T2")
                            {
                                branch = "L3";
                                rating = 1000 * rating;
                            }
                            else if (type == "banner" and isOnClick == "false" and qbranch == "T3")
                            {
                                branch = "L4";
                                rating = 1000 * rating;
                            }
                            else if (type == "banner" and isOnClick == "false" and qbranch == "T4")
                            {
                                branch = "L5";
                                rating = 1000 * rating;
                            }
                            else if (type == "banner" and isOnClick == "false" and qbranch == "T5")
                            {
                                branch = "L6";
                                rating = 1000 * rating;
                            }
                            else if (type == "banner" and isOnClick == "true" and qbranch == "T1")
                            {
                                branch = "L7";
                            }
                            else if (type == "banner" and isOnClick == "true" and qbranch == "T2")
                            {
                                branch = "L8";
                            }
                            else if (type == "banner" and isOnClick == "true" and qbranch == "T3")
                            {
                                branch = "L9";
                            }
                            else if (type == "banner" and isOnClick == "true" and qbranch == "T4")
                            {
                                branch = "L10";
                            }
                            else if (type == "banner" and isOnClick == "true" and qbranch == "T5")
                            {
                                branch = "L11";
                            }
                            else if (type == "teaser" and isOnClick == "false" and qbranch == "T1")
                            {
                                branch = "L12";
                            }
                            else if (type == "teaser" and isOnClick == "false" and qbranch == "T2")
                            {
                                branch = "L13";
                            }
                            else if (type == "teaser" and isOnClick == "false" and qbranch == "T3")
                            {
                                branch = "L14";
                            }
                            else if (type == "teaser" and isOnClick == "false" and qbranch == "T4")
                            {
                                branch = "L15";
                            }
                            else if (type == "teaser" and isOnClick == "false" and qbranch == "T5")
                            {
                                branch = "L16";
                            }
                            else if (type == "teaser" and isOnClick == "true" and qbranch == "T1")
                            {
                                branch = "L17";
                            }
                            else if (type == "teaser" and isOnClick == "true" and qbranch == "T2")
                            {
                                branch = "L18";
                            }
                            else if (type == "teaser" and isOnClick == "true" and qbranch == "T3")
                            {
                                branch = "L19";
                            }
                            else if (type == "teaser" and isOnClick == "true" and qbranch == "T4")
                            {
                                branch = "L20";
                            }
                            else if (type == "teaser" and isOnClick == "true" and qbranch == "T5")
                            {
                                branch = "L21";
                            }
                            else
                            {
                                Log::warn("Результат лишний: %s", guid.c_str());
                                break;
                            }
                            if (match == "nomatch")
                            {
                                matching = (string) sphinx_get_string( res, i, 0 ) + " | " + (string) sphinx_get_string( res, i, 1 );
                            }
                            else if (match == "broadmatch")
                            {
                                matching = sphinx_get_string( res, i, 2 );
                            }
                            else if (match == "phrasematch")
                            {
                                matching = sphinx_get_string( res, i, 4 );
                            }
                            else if (match == "exactmatch")
                            {
                                matching = sphinx_get_string( res, i, 3 );
                            }
                            else
                            {
                                Log::warn("Результат лишний %s", guid.c_str());
                                break;
                            }
                            I->second = pair<pair<float,pair<pair<string,string>,pair<string,string>>>,pair<pair<string,string>,pair<string,string>>>(
                                            pair<float,pair< pair<string,string>, pair<string,string> > >(
                                                rating,
                                                pair<pair<string,string>,pair<string,string>>(
                                                    pair<string,string>(branch,"false"),
                                                    pair<string,string>(type,isOnClick)
                                                )
                                            ),
                                            pair<pair<string,string>,pair<string,string>>(
                                                pair<string,string>(match,matching),
                                                pair<string,string>("false","")
                                            )
                                        );
                        }
                    }
                }
                else
                {
                    Log::warn("Нехватает атрибутов");
                }
            }
            res++;
        }
        //LOG(INFO) << "Изменения массива предложений заняло: " << (int32_t)(Misc::currentTimeMillis() - strr) << " мс";
        sphinx_reset_filters ( client );
        delete [] rate;
        delete [] branches;
        delete [] filter;
    }
    catch (std::exception const &ex)
    {
        Log::err("Непонятная sphinx ошибка: %s : %s", typeid(ex).name(), ex.what());
    }
    //LOG(INFO) << "Выход из обработки";

    return ;
}
