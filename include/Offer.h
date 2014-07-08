#ifndef OFFER_H
#define OFFER_H

#include <string>
#include <list>
#include <map>
#include <vector>

#include "Campaign.h"
#include "EBranch.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

/** \brief  Класс описывает рекламное предложение (например, товар или новость). */
class Offer
{
public:
    typedef std::map <const unsigned long,Offer*> Map;
    typedef std::multimap <const float,Offer*, std::greater<float>> MapRate;
    typedef std::map <const unsigned long,Offer*>::iterator it;
    typedef std::map <const unsigned long,Offer*>::const_iterator cit;
    typedef std::pair<const float,Offer*> PairRate;
    typedef std::pair<const unsigned long,Offer*> Pair;
    typedef std::vector <Offer*> Vector;
    typedef std::vector <Offer*>::iterator itV;

    typedef enum { banner, teazer, unknown } Type;
    /// Структура для хранения информации о рекламном предложении.
    std::string id;             ///< ID предложения
    unsigned long long id_int;                 ///< ID предложения
    std::string title;          ///< Заголовок
    std::string price;          ///< Цена
    std::string description;    ///< Описание
    std::string url;            ///< URL перехода на предложение
    std::string image_url;      ///< URL картинки
    std::string swf;            ///< URL Flash
    unsigned long long campaign_id;    ///< ID кампании, к которой относится предложение
    bool isOnClick;             ///< Реклама по кликам или показам
    Type type;			///< тип РП
    std::string conformity;		///< тип соответствия РП и запроса, изменяеться после выбора РП из индекса
    EBranchL branch;			///< ветка алгоритма по которой было выбрано РП
    std::string matching;       ///< Фраза соответствия
    float rating;				///< рейтинг РП
    bool retargeting;
    int uniqueHits;		///< максимальное количество показов одному пользователю
    unsigned height;					///< высота РП (имеет значение для баннеров)
    unsigned width;					///< ширина РП (имеет значение для баннеров)
    bool social;
    std::string campaign_guid;
    unsigned unique_by_campaign;

    long long int token_int;
    std::string token;          ///< Токен для проверки ссылки
    std::string redirect_url;   ///< Cсылка перенаправления
    unsigned showCount;

    //Offer(){};

    Offer(const std::string &id,
          long long id_int,
          const std::string &title,
          const std::string &price,
          const std::string &description,
          const std::string &url,
          const std::string &image_url,
          const std::string &swf,
          long long campaign_id,
          bool isOnClick,
          int type,
          float rating,
          bool retargeting,
          int uniqueHits,
          int height,
          int width,
          bool social,
          std::string campaign_guid,
          int unique_by_campaign);

    virtual ~Offer();

    //bool operator==(const Offer &other) const { return this->id_int == other.id_int; }
    //bool operator<(const Offer &other) const { return rating < other.rating; }
    /*
        bool operator<(const Offer *x1, const Offer *x2)
        {
            return x1->rating > x2->rating;
        }
    */
    static Type typeFromString(const std::string &stype)
    {
        if(stype == "banner")
            return Type::banner;
        if(stype == "teaser")
            return Type::teazer;
        return Type::unknown;
    }

    static Type typeFromInt(int stype)
    {
        switch(stype)
        {
        case 0:
            return Type::banner;
        case 1:
            return Type::teazer;
        default:
            return Type::unknown;
        }
    }

    static std::string typeToString(const Type &stype)
    {
        switch(stype)
        {
            case Type::banner:
                return "banner";
            case Type::teazer:
                return "teaser";
            default:
                return "unknown";
        }
    }

    // Каждому элементу просмотра присваиваем уникальный токен
    std::string gen();

    std::string toJson() const;
    bool setBranch(const  EBranchT tbranch);
    std::string getBranch() const
    {
        return EBranchL2String(branch);
    };
};

#endif // OFFER_H

