#ifndef CAMPAIGN_H
#define CAMPAIGN_H

#include <string>
#include <map>
#include <list>

#include "KompexSQLiteDatabase.h"

typedef long long			sphinx_int64_t;
typedef unsigned long long	sphinx_uint64_t;


/**
  \brief  Класс, описывающий рекламную кампанию
*/
class Campaign
{
public:
    Campaign(const std::string &id);

    /** \brief Загружает информацию обо всех кампаниях */
    static void loadAll(Kompex::SQLiteDatabase *pdb);

    /** \brief Возвращает идентификатор рекламной кампании */
    std::string id() const
    {
        return d->id;
    }
    sphinx_int64_t id_int() const
    {
        return d->id_int;
    }

    /** \brief Возвращает название рекламной кампании */
    std::string title() const
    {
        return d->title;
    }

    /** \brief Возвращает название проэкта которому пренадлежит рекламная кампания */
    std::string project() const
    {
        return d->project;
    }

    /** \brief Является ли кампания социальной.

    Переходы по социальной рекламе не засчитываются владельцам сайтов.
    Социальная реклама показывается в том случае, если при данных условиях
    (страна, время и т.д.) больше нечего показать.

    По умолчанию равно false (кампания является коммерческой).
      */
    bool social() const
    {
        return d->social;
    }

    /** \brief Возвращает true, если кампания действительна (была успешно загружена) */
    bool valid() const
    {
        return d->valid;
    }

    /** \brief Возвращает список всех кампаний */
    static std::list<Campaign> &all()
    {
        static std::list<Campaign> all_;
        return all_;
    }


    bool operator==(const Campaign &other) const
    {
        return this->d == other.d;
    }
    bool operator<(const Campaign &other) const
    {
        return this->d < other.d;
    }

private:

};

#endif // CAMPAIGN_H
