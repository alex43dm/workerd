#ifndef OFFER_H
#define OFFER_H

#include <string>
#include <list>
#include <map>

#include <mongo/client/dbclientinterface.h>

#include "Campaign.h"
#include "KompexSQLiteDatabase.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

/** \brief  Класс описывает рекламное предложение (например, товар или новость). */
class Offer
{
public:
    /// Структура для хранения информации о рекламном предложении.
        std::string id;             ///< ID предложения
        sphinx_int64_t id_int;                 ///< ID предложения
        std::string title;          ///< Заголовок
        std::string price;          ///< Цена
        std::string description;    ///< Описание
        std::string url;            ///< URL перехода на предложение
        std::string image_url;      ///< URL картинки
        std::string swf;            ///< URL Flash
        std::string campaign_id;    ///< ID кампании, к которой относится предложение
        bool valid;                 ///< Является ли запись действительной.
        bool isOnClick;             ///< Реклама по кликам или показам
        std::string type;			///< тип РП
        std::string conformity;		///< тип соответствия РП и запроса, изменяеться после выбора РП из индекса
        std::string branch;			///< ветка алгоритма по которой было выбрано РП
        std::string matching;       ///< Фраза соответствия
        float rating;				///< рейтинг РП
        int uniqueHits;				///< максимальное количество показов одному пользователю
        int height;					///< высота РП (имеет значение для баннеров)
        int width;					///< ширина РП (имеет значение для баннеров)
        bool social;
        bool isBanner;

        std::string token;          ///< Токен для проверки ссылки
        std::string redirect_url;   ///< Cсылка перенаправления


    Offer(){};

    Offer(const std::string &id,
          long id_int,
          const std::string &title,
          const std::string &price,
          const std::string &description,
          const std::string &url,
          const std::string &image_url,
          const std::string &swf,
          const std::string &campaign_id,
          bool valid,
          bool isOnClick,
          const std::string &type,
          float rating,
          int uniqueHits,
          int height,
          int width);

    virtual ~Offer();

    static void loadAll(Kompex::SQLiteDatabase *pdb);

    bool operator==(const Offer &other) const { return *this == other; }
    bool operator<(const Offer &other) const { return rating < other.rating; }

    // Каждому элементу просмотра присваиваем уникальный токен
    void gen();

    std::string toJson() const;
};

#endif // OFFER_H
