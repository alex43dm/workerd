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
//        sphinx_int64_t id_int;                             //Индентификатор РБ
        int capacity;                           //Количество мест под тизер
        std::string bannersCss;                 //Стиль CSS РБ для отображения банеров
        std::string teasersCss;                 //Стиль CSS РБ для отображения тизеров
        long domainId;
        long accountId;
        //unsigned rtgPercentage;
        unsigned RetargetingCount;

        bool blocked;                           //Статус активности РБ
        ShowNonRelevant nonrelevant;            //Что отображать при отсутствии платных РП
        std::string user_code;                  //Строка пользовательского кода
        bool valid;                             //Валидность блока
        int height;                             //Высота блока
        int width;                              //Ширина блока
        int height_banner;                      //Высота отображаемых банеров
        int width_banner;                       //Ширина отображаемых банеров

    Informer(long id);
    Informer(long id, int capacity, const std::string &bannersCss, const std::string &teasersCss, long , long);//, int);
    virtual ~Informer();

    bool is_null() const
    {
        return id==0;
    }

    bool operator==(const Informer &other) const;
    bool operator<(const Informer &other) const;

private:

};

#endif // INFORMER_H
