/**
 Класс добавлен RealInvest Soft
 */
#pragma once
#include <string>
#include <sstream>
#include <list>
using namespace std;

/** \brief Класс, содержащий в себе истории пользователя (short, long) и список идентификаторов РП,
	запрещённых к показы конкретному пользователю. */
class UserHistory
{
private:
    ///Идентификатор пользователя
    string id_user;

    ///Краткосрочная история пользователя
    list<string> shortHistory;

    ///Долгосрочная история пользователя
    list<string> longHistory;

    ///Список рекламных предложений, запрещенных к показу для данного пользователя
    list<string> deprecatedOffers;

    ///История контекста страниц пользователя
    list<string> contextHistory;

    ///История категорий пользователя
    list<string> categoryHistory;

    ///История
    list<string> retargetingHistory;
public:
    /** \brief  Конструктор.

        \param id_user_        идентификатор пользователя
    */
    UserHistory(const string& id_user_);
    ~UserHistory();

    /** \brief  Добавление запроса к краткосрочной истории

        \param keyWord       строка запроса
    */
    void addShortHistory(const string& keyWord);

    /** \brief  Добавление ключевой фразы к долгосрочной истории

        \param keyWord       ключевая фраза
    */
    void addLongHistory(const string& keyWord);

    /** \brief  Добавление идентификатора рекламного предложения, запрещенного к показу

        \param offerId       идентификатор рекламного предложения
    */
    void addDeprecatedOffer(const string& offerId);

    /** \brief  Добавление контекстных слов к контекстной истории

        \param keyWord      контекстные слова
    */
    void addContextHistory(const string& keyWord);

    /** \brief  Добавление категорий

        \param catId      категории
    */
    void addCategory(const string& catId);

    /** \brief  Добавление категорий

        \param catId      категории
    */
    void addRetargeting(const string& retOfferId);

    /** \brief  Возвращает краткосрочную историю */
    list<string>& getShortHistory()
    {
        return shortHistory;
    }

    /** \brief  Возвращает долгосрочную историю */
    list<string>& getLongHistory()
    {
        return longHistory;
    }

    /** \brief  Возвращает историю показов*/
    list<string>& getDeprecatedOffers()
    {
        return deprecatedOffers;
    }

    /** \brief  Возвращает контекстную историю*/
    list<string>& getContextHistory()
    {
        return contextHistory;
    }

    /** \brief  Возвращает историю категорий*/
    list<string>& getCategoryHistory()
    {
        return categoryHistory;
    }

    /** \brief  Возвращает историю */
    list<string>& getRetargetingHistory()
    {
        return retargetingHistory;
    }

    string historyToLog();

    /** \brief  Возвращает историю пользователя в формате Json*/
    std::string UserHistoryToJson(std::string query);

};
