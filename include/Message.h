#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>

class Message
{
    public:
        int fd;

        Message(int);
        virtual ~Message();
    protected:
    private:
};

#endif // MESSAGE_H
