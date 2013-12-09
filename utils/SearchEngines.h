#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <cstring>
#include <iostream>
using namespace std;

/** \brief Добавлено RealInvest Soft. Структура, хранящая соответствия поисковиков и названий параметров, с которых начинается строка запроса пользователя.

 Реализует шаблон Singleton.
 Данные хранятся в map.
 */
struct SearchEngineMapContainer
{
private:
    /** \brief Закрытый конструктор. */
    SearchEngineMapContainer() {}

    map<string, vector<string> > m_listSSearchEngines;///< map, хранящий соответствие между поисковиком и значением параметра, за которым следует строка запроса пользователя
public:
    /** \brief Метод получения экземпляра класса. */
    static SearchEngineMapContainer* instance()
    {
        static SearchEngineMapContainer *ob = new SearchEngineMapContainer;
        return ob;
    }

    /** \brief Метод для считывания данных из файла.
     \param filename Имя файла, из которого будут считываться данные.
    @return false, если не удалось открыть файл filename,\n true, если инициализация прошла успешно.

    Пример записи данных в файле:
    \code
    google.: q=, as_q=, as=
    rambler.ru: query=, words=
    go.mail.ru/search_images: q=
    go.mail.ru: q=
    images.google.: q=
    search.live.com: q=
    rapidshare-search-engine: s=
    search.yahoo.com: p=
    nigma.ru/index.php: s=, q=
    search.msn.com/results: q=
    ask.com/web: q=
    search.qip.ru/search: query=
    rapidall.com/search.php: query=
    images.yandex.ru/: text=
    m.yandex.ru/search: query=
    hghltd.yandex.net: text=
    yandex.ru: text=
    \endcode
    */
    bool setSearchEnginesMap(const string &filename);

    /** \brief Возвращает map m_listSSearchEngines. */
    map<string, vector<string> >& getMap()
    {
        return m_listSSearchEngines;
    }
};
