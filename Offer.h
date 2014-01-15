#ifndef OFFER_H
#define OFFER_H

#include <string>
#include <list>
#include <map>
#include <vector>

#include <mongo/client/dbclientinterface.h>

#include "Campaign.h"
#include "KompexSQLiteDatabase.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

/** \brief  Класс описывает рекламное предложение (например, товар или новость). */
class Offer
{
public:
    typedef std::map <const long,Offer*> Map;
    typedef std::map <const long,Offer*>::iterator it;
    typedef std::map <const long,Offer*>::const_iterator cit;
    typedef std::pair<const long,Offer*> Pair;

    typedef enum{ banner, teazer, unknown } Type;
    /// Структура для хранения информации о рекламном предложении.
        std::string id;             ///< ID предложения
        long long id_int;                 ///< ID предложения
        std::string title;          ///< Заголовок
        std::string price;          ///< Цена
        std::string description;    ///< Описание
        std::string url;            ///< URL перехода на предложение
        std::string image_url;      ///< URL картинки
        std::string swf;            ///< URL Flash
        long long campaign_id;    ///< ID кампании, к которой относится предложение
        bool valid;                 ///< Является ли запись действительной.
        bool isOnClick;             ///< Реклама по кликам или показам
        Type type;			///< тип РП
        std::string conformity;		///< тип соответствия РП и запроса, изменяеться после выбора РП из индекса
        std::string branch;			///< ветка алгоритма по которой было выбрано РП
        std::string matching;       ///< Фраза соответствия
        float rating;				///< рейтинг РП
        bool retargeting;
        int uniqueHits;				///< максимальное количество показов одному пользователю
        int height;					///< высота РП (имеет значение для баннеров)
        int width;					///< ширина РП (имеет значение для баннеров)
        bool social;
        bool isBanner;

        std::string token;          ///< Токен для проверки ссылки
        std::string redirect_url;   ///< Cсылка перенаправления

    Offer(){};

    Offer(const std::string &id,
          long long id_int,
          const std::string &title,
          const std::string &price,
          const std::string &description,
          const std::string &url,
          const std::string &image_url,
          const std::string &swf,
          long long campaign_id,
          bool valid,
          bool isOnClick,
          int type,
          float rating,
          bool retargeting,
          int uniqueHits,
          int height,
          int width);

    virtual ~Offer();

    static void loadAll(Kompex::SQLiteDatabase *pdb, mongo::Query=mongo::Query());

    bool operator==(const Offer &other) const { return this->id_int == other.id_int; }
    bool operator<(const Offer &other) const { return rating < other.rating; }

    static Type typeFromString(const std::string stype)
    {
        if(stype == "banner")
            return Type::banner;
        if(stype == "teaser")
            return Type::teazer;
        return Type::unknown;
    }

    static Type typeFromInt(int stype)
    {
        if(stype == 0)
            return Type::banner;
        if(stype == 1)
            return Type::teazer;
        return Type::unknown;
    }

    // Каждому элементу просмотра присваиваем уникальный токен
    void gen();

    std::string toJson() const;
    bool setBranch(const std::string &qbranch);
};

class OfferExistByType
{
    Offer::Type type;
public:
    OfferExistByType(Offer::Type t):type(t){}

    bool operator()(const Offer::Pair &temp)
    {
            return temp.second->type == type;
    }
};

#endif // OFFER_H
