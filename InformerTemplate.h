#ifndef INFORMERTEMPLATE_H
#define INFORMERTEMPLATE_H

#include <string>

/**

	\brief Класс, отвечающий за хранение шаблонов для информеров.

	Хранятся 2 шаблона: для тизеров (существовавший) и для баннеров (новый).
	Отдельно хранится текст библиотеки swfobject, который считывается из файла swfobject.js.
    Он нужен для отображения баннера (для шаблона для баннеров), т.к. втраивание flash-баннера в страницу сделано с помощью этой библиотеки.
    При написании и отладке молуля использовалась версия SWFObject v2.2.
*/
class InformerTemplate
{
private:
    InformerTemplate() {}
protected:
    std::string teasersTemplate;
    std::string bannersTemplate;
    std::string swfobjectLibStr;

    bool initTeasersTemplate();
    /** filename - имя файла с библиотекой swfobject */
    bool initBannersTemplate(const std::string& filename);
public:
    static InformerTemplate* instance()
    {
        static InformerTemplate *ob = new InformerTemplate;
        return ob;
    }

    bool init();

    /** Шаблон информера с тизерами со следующими подстановками (существовавший шаблон):

    %1%	    CSS
    %2%	    JSON для initads (предложения, которые должны показаться на первой
            странице)
    %3%	    количество товаров на странице
    %4%	    ссылка на скрипт получения следующей порции предложений в json,
            к ней будут добавляться дополнительные параметры.
    */
    const std::string& getTeasersTemplate()
    {
        return teasersTemplate;
    }
    /** Шаблон информера с баннером со следующими подстановками:

    %1%	    CSS
    %2%	    swfobject (пришлось делать так, ибо в текстве библиотеки есть символ '%' и boost думает, что туда надо подставлять,
            что приводит к ошибке во время выполнения программы). swfobject можно получить у InformerTemplate с помощью метода getSwfobjectLibStr().
    %3%	    JSON для initads (баннер)
    */
    const std::string& getBannersTemplate()
    {
        return bannersTemplate;
    }
    /** строка с библиотекой swfobject */
    const std::string& getSwfobjectLibStr()
    {
        return swfobjectLibStr;
    }
};

#endif // INFORMERTEMPLATE_H
