#ifndef COMPARATORS_H
#define COMPARATORS_H

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <stdlib.h>
using namespace std;




/**
	добавлено RealInvest Soft
	функции сравнения Offer
*/
bool rigthOrderCompare(const Offer &a, const Offer &b)
{
    return a.rating < b.rating;
}

bool inverseOrderCompare(const Offer &a, const Offer &b)
{
    return a.rating > b.rating;
}


/**
	добавлено RealInvest Soft
	функции поиска строки в списке или векторе
*/
bool isStrInList(const string& str, const list<string>& l)
{
    list<string>::const_iterator it = l.begin();

    it = std::find(l.begin(),l.end(), str);
    return it != l.end();
}
bool isStrInVector(const string& str, const vector<string>& v)
{
    vector<string>::const_iterator it = v.begin();

    it = std::find(v.begin(),v.end(), str);
    return it != v.end();
}

bool isOfferInVector(const Offer *offer, const vector<Offer*> &v)
{
        for (size_t i=0; i<v.size(); i++)
        {
                if (offer==v[i])
                {
                    return true;
                }
        }
        return false;
}

enum EOfferData
{
    EOD_ID,
    EOD_TITLE,
    EOD_PRICE,
    EOD_DESCRIPTION,
    EOD_URL,
    EOD_IMAGE_URL,
    EOD_CAMPAIGN_ID,
    EOD_TYPE,
    EOD_RATING
};

/**
	добавлено RealInvest Soft
	функтор для поиска по типу предложения
*/
class CExistElementFunctorByType
{
    string m_str;
    Offer::Type type;
    EOfferData m_EOfferData;
public:
    CExistElementFunctorByType(string str, EOfferData eOfferData)
    {
        m_str = str;
        m_EOfferData = eOfferData;
    }

    CExistElementFunctorByType(Offer::Type t, EOfferData eOfferData)
    {
        type = t;
        m_EOfferData = eOfferData;
    }

    bool operator()(Offer *temp)
    {
        switch(m_EOfferData)
        {
        case EOD_ID:
            return temp->id == m_str;

        case EOD_TITLE:
            return temp->title == m_str;

        case EOD_PRICE:
            return temp->price == m_str;

        case EOD_DESCRIPTION:
            return temp->description == m_str;

        case EOD_URL:
            return temp->url == m_str;

        case EOD_IMAGE_URL:
            return temp->image_url == m_str;

        case EOD_CAMPAIGN_ID:
            return temp->campaign_id == m_str;

        case EOD_TYPE:
            return temp->type == type;

        case EOD_RATING:
            return temp->rating == atof(m_str.c_str());

        default:
            return true;
        }
    }
};

#endif
