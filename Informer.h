#ifndef INFORMER_H
#define INFORMER_H

#include <string>
#include <map>
#include <set>

#include "KompexSQLiteDatabase.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

// Forward declarations
namespace mongo
{
class Query;
}

/**
    \brief Класс, описывающий рекламную выгрузку
*/
class Informer
{
public:
    enum ShowNonRelevant
    {
        Show_Social,
        Show_UserCode
    };

public:
        long long id;                         //Индентификатор РБ
        sphinx_int64_t id_int;                             //Индентификатор РБ
        std::string title;                      //Название РБ
        std::string teasersCss;                 //Стиль CSS РБ для отображения тизеров
        std::string bannersCss;                 //Стиль CSS РБ для отображения банеров
        std::string domain;                     //Доменное имя за которым заркеплён РБ
        std::string user;                       //Логин аккаунта Getmyad за которым заркеплён РБ
        bool blocked;                           //Статус активности РБ
        std::set<std::string> categories;       //Принадлежность РБ к категории
        ShowNonRelevant nonrelevant;            //Что отображать при отсутствии платных РП
        std::string user_code;                  //Строка пользовательского кода
        int capacity;                           //Количество мест под тизер
        bool valid;                             //Валидность блока
        int height;                             //Высота блока
        int width;                              //Ширина блока
        int height_banner;                      //Высота отображаемых банеров
        int width_banner;                       //Ширина отображаемых банеров
        long domainId;
        long accountId;

    Informer(long id);
    Informer(long id, int capacity, const std::string &bannersCss, const std::string &teasersCss, long , long);
    virtual ~Informer();

    /** Загружает информацию обо всех информерах */
    static bool loadAll(Kompex::SQLiteDatabase *pdb);

    bool is_null() const
    {
        return id==0;
    }

    bool operator==(const Informer &other) const;
    bool operator<(const Informer &other) const;

private:

};

#endif // INFORMER_H
