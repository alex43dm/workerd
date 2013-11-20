#ifndef MAINDB_H
#define MAINDB_H

#include <fastdb/fastdb.h>

USE_FASTDB_NAMESPACE

class Campaign
{
public:
    int8 id;     //ID кампании.
    const char *title;  //Название кампании.
    bool social;        //Является ли кампания социальной.
    bool valid;
    TYPE_DESCRIPTOR((KEY(id, INDEXED|HASHED), FIELD(title), FIELD(social), FIELD(valid)));
};

class Offer
{
public:
    int8 id;         //ID предложения.
    const char *title;      //Заголовок.
    const char *price;      //Цена.
    const char *description;//Описание.
    const char *url;        //URL перехода на предложение.
    const char *image_url;  //URL картинки.
    const char *campaign_id;//ID кампании, к которой относится предложение.
    bool 	valid;

    TYPE_DESCRIPTOR((KEY(id, INDEXED|HASHED), FIELD(title), FIELD(price),
                     FIELD(description), FIELD(url), FIELD(image_url), FIELD(campaign_id), FIELD(valid) ));
};

class Informer
{
public:
    int8 id;         //ID.
    const char *title;      //Наименование информера.
    const char *css;        //CSS стиль информера.
    const char *domain;     //Домен, к которому принадлежит информер.
    const char *user;       //Пользователь, которому принадлежит информер.
    bool 	blocked;        //Заблокирован ли информер.
    const char *categories; //Множество категорий, к которым относится информер.
    int1    nonrelevant;    //Действие при отстутсвии релевантной рекламы.
    const char *user_code;
    int1 	capacity;       //Количество предложений на информере.
    bool 	valid;          //Действительна ли запись.

    TYPE_DESCRIPTOR((KEY(id, INDEXED|HASHED), FIELD(title), FIELD(css),
                     FIELD(domain), FIELD(user), FIELD(blocked), FIELD(categories)
                     , FIELD(nonrelevant), FIELD(user_code), FIELD(capacity), FIELD(valid) ));
};

class MainDb
{
public:
    dbDatabase *db;

    MainDb(const std::string &path);
    virtual ~MainDb();
    bool gen(int from, int len);
    bool get();
protected:
private:

};

#endif // MAINDB_H
