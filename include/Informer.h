#ifndef INFORMER_H
#define INFORMER_H

#include <string>

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;

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

       long long id;                         //Индентификатор РБ
        unsigned capacity;                           //Количество мест под тизер
        std::string bannersCss;                 //Стиль CSS РБ для отображения банеров
        std::string teasersCss;                 //Стиль CSS РБ для отображения тизеров
        long domainId;
        long accountId;
        unsigned retargeting_capacity;
        double range_short_term, range_long_term, range_context, range_search, range_category;

        bool blocked;                           //Статус активности РБ
        ShowNonRelevant nonrelevant;            //Что отображать при отсутствии платных РП
        std::string user_code;                  //Строка пользовательского кода
        bool valid;                             //Валидность блока
        int height;                             //Высота блока
        int width;                              //Ширина блока
        int height_banner;                      //Высота отображаемых банеров
        int width_banner;                       //Ширина отображаемых банеров

    Informer(long id);
    Informer(long id, int capacity,
             const std::string &bannersCss,
             const std::string &teasersCss, long , long, double, double, double, double, double, int, bool);//, int);
    virtual ~Informer();

    bool is_null() const
    {
        return id==0;
    }

    bool operator==(const Informer &other) const;
    bool operator<(const Informer &other) const;

    bool isShortTerm() const {return range_short_term > 0;}
    bool isLongTerm() const {return range_long_term > 0;}
    bool isContext() const {return range_context > 0;}
    bool isSearch() const {return range_search > 0;}
    bool isCategory() const {return range_category > 0;}
    bool sphinxProcessEnable() const {
        return range_short_term > 0 || range_long_term > 0 || range_context > 0 || range_search > 0
        || range_category > 0;}

private:

};

#endif // INFORMER_H
