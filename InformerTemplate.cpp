#include <fstream>

#include "Config.h"
#include "InformerTemplate.h"

std::string InformerTemplate::getFileContents(const std::string &fileName)
{
    std::string cnt;
    std::ifstream in(fileName, std::ios::in | std::ios::binary);
    if(in)
    {
        in.seekg(0, std::ios::end);
        cnt.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&cnt[0], cnt.size());
        in.close();
        return(cnt);
    }

    throw(errno);
}

/** Шаблон информера с тизерами со следующими подстановками (существовавший шаблон):

    %1%	    CSS
    %2%	    JSON для initads (предложения, которые должны показаться на первой
	    странице)
    %3%	    количество товаров на странице
    %4%	    ссылка на скрипт получения следующей порции предложений в json,
	    к ней будут добавляться дополнительные параметры.
*/
bool InformerTemplate::initTeasersTemplate()
{
    if (teasersTemplate!="")
    {
        return true;
    }

    teasersTemplate = getFileContents(Config::Instance()->template_teaser_);

    return true;
}



/** Шаблон информера с баннером со следующими подстановками:

    %1%	    CSS
    %2%	    swfobject (пришлось делать так, ибо в текстве библиотеки есть символ '%' и boost думает, что туда надо подставлять,
            что приводит к ошибке во время выполнения программы). swfobject можно получить у InformerTemplate с помощью метода getSwfobjectLibStr().
	%3%	    JSON для initads (баннер)
*/
bool InformerTemplate::initBannersTemplate()
{
    if (bannersTemplate!="")
    {
        return true;
    }
    //считали библиотеку успешно. инициализируем шаблон весь.
    swfobjectLibStr = getFileContents(Config::Instance()->swfobject_);
    bannersTemplate = getFileContents(Config::Instance()->template_banner_);
    return true;
}


bool InformerTemplate::init()
{
    return (initTeasersTemplate() && initBannersTemplate());
}
