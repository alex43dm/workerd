#ifndef RANDOMENTITY_H
#define RANDOMENTITY_H

//#include <map>
#include <list>
#include <cstdlib>
#include <ctime>
#include <algorithm>

/** \brief Класс, служащий для случайного выбора элементов с учётом их
    весового распределения
*/
template<class T>
class RandomEntity
{
    struct Record
    {
        T element;
        double weight;

        Record(const T &element, double weight)
            : element(element), weight(weight) { }
    };
    class Record_less
    {
    public:
        bool operator()(Record *&arg1, const double &arg2) const
        {
            return arg1->weight < arg2;
        }
    };

public:
    RandomEntity()
        : summ_(0)
    {
        static bool first_init = true;
        if (first_init)
        {
            srand( time(NULL) );
            first_init = false;
        }
    }

    class NoItemsException : public std::exception
    {
    };

    /** Добавляет вариант выбора элемента element с весовым
    распределением weight
    */
    void add(const T &element, double weight)
    {
        summ_ += weight;
        Record *r = new Record(element, summ_);
        items_.push_back(r);
    }

    /** Удаляет element из распределения */
    void remove(const T &element)
    {
        for (auto it = items_.begin(); it != items_.end(); it++)
        {
            if ((*it)->element == element)
            {
                items_.erase(it);
                return;
            }
        }
    }

    /** Возвращает случайно выбранный элемент */
    T get()
    {
        double r = double(rand()) / RAND_MAX * summ_;
        typename std::list<Record*>::iterator it;
        it = std::lower_bound(items_.begin(), items_.end(), r, Record_less());
        if (it != items_.end())
            return (*it)->element;
        else
            throw NoItemsException();
    }


    /** Количество элементов, из которых производится выбор */
    int count()
    {
        return items_.size();
    }

private:
    std::list<Record *> items_;
    double summ_;
};



#endif // RANDOMENTITY_H
