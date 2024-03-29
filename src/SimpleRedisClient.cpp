/*
 * File:   SimpleRedisClient.cpp
 * Author: victor
 *
 * Created on 10 Август 2013 г., 22:26
 */

#include <cstdlib>
#include <iostream>
#include <memory>

#include <pthread.h>

#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SimpleRedisClient.h"
#include "Log.h"


/**
 * Читает целое число из строки, если ошибка то вернёт -1
 * @param buffer Строка
 * @param delimiter Конец для числа
 * @param delta Количество символов занятое числом и разделителем
 * @return
 */
int read_int(const char* buffer,char delimiter,int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 9)
        {
            return -1;
        }
    }
    return len;
}

int read_int(const char* buffer,char delimiter)
{
    const char* p = buffer;
    int len = 0;
    int delta = 0;

    while(*p == '0' )
    {
        p++;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        delta++;
        if(delta > 9)
        {
            return -1;
        }
    }

    return len;
}

int read_int(const char* buffer, int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(1)
    {
        if(*p > '9' || *p < '0')
        {
            return len;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 9)
        {
            return -1;
        }
    }
    return len;
}
/**
 * Читает целое число из строки, если ошибка то вернёт -1
 * @param buffer Строка
 * @param delimiter Конец для числа
 * @param delta Количество символов занятое числом и разделителем
 * @return
 */
long read_long(const char* buffer,char delimiter,int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 18)
        {
            return -1;
        }
    }
    return len;
}

long read_long(const char* buffer,char delimiter)
{
    const char* p = buffer;
    int len = 0;
    int delta = 0;

    while(*p == '0' )
    {
        p++;
    }

    while(*p != delimiter)
    {
        if(*p > '9' || *p < '0')
        {
            return -1;
        }

        len = (len*10)+(*p - '0');
        p++;
        delta++;
        if(delta > 18)
        {
            return -1;
        }
    }

    return len;
}

long read_long(const char* buffer, int* delta)
{
    const char* p = buffer;
    int len = 0;
    int d = 0;

    while(*p == '0' )
    {
        (*delta)++;
        p++;
        //return 0;
    }

    while(1)
    {
        if(*p > '9' || *p < '0')
        {
            return len;
        }

        len = (len*10)+(*p - '0');
        p++;
        (*delta)++;
        d++;

        if(d > 18)
        {
            return -1;
        }
    }
    return len;
}

#define RC_ERROR '-'

/**
 * Строка в ответе
 * @see http://redis.io/topics/protocol
 */
#define RC_INLINE '+'

/**
 * Определяет длину аргумента ответа
 * -1 нет ответа
 * @see http://redis.io/topics/protocol
 */
#define RC_BULK '$'


/**
 * Определяет количество аргументов ответа
 * -1 нет ответа
 * @see http://redis.io/topics/protocol
 */
#define RC_MULTIBULK '*'
#define RC_INT ':'
#define RC_ANY '?'
#define RC_NONE ' '


#define REDIS_PRINTF_MACRO_CODE(type, comand) va_list ap;\
                        va_start(ap, format);\
                        bzero(buf, buffer_size);\
                        int  rc = vsnprintf(buf, buffer_size, format, ap);\
                        va_end(ap);\
                        if( rc >= buffer_size ) return RC_ERR_BUFFER_OVERFLOW;\
                        if(rc <  0) return RC_ERR_DATA_FORMAT;\
                        rc = redis_send( type, "%s %s\r\n", comand, buf);\
                        return rc;\


SimpleRedisClient::SimpleRedisClient(const std::string &Host,
                                     const std::string &Port,
                                     const std::string &Name)
{
    setBufferSize(2048);
    host = Host;
    port = strtol(Port.c_str(),NULL,10);
    name = "redis("+Name+"): ";
}

int SimpleRedisClient::getBufferSize()
{
    return buffer_size;
}


void SimpleRedisClient::setMaxBufferSize(int size)
{
    max_buffer_size = size;
}

int SimpleRedisClient::getMaxBufferSize()
{
    return max_buffer_size;
}

void SimpleRedisClient::setBufferSize(int size)
{
    if(buffer != 0)
    {
        delete[] buffer;
        delete[] buf;
    }

    buffer_size = size;
    buffer = new char[buffer_size];
    buf = new char[buffer_size];
}

SimpleRedisClient::~SimpleRedisClient()
{
    redis_close();


    if(buffer != NULL)
    {
        delete[] buffer;
    }

    if(buf != NULL)
    {
        delete[] buf;
    }

    buffer_size = 0;
}

/*
 * Returns:
 *  >0  Колво байт
 *   0  Соединение закрыто
 *  -1  error
 *  -2  timeout
 **/
int SimpleRedisClient::read_select(int fd, int timeout ) const
{
    struct timeval tv;
    fd_set fds;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000)*1000;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    return select(fd + 1, &fds, NULL, NULL, &tv);
}

/*
 * Returns:
 *  >0  Колво байт
 *   0  Соединение закрыто
 *  -1  error
 *  -2  timeout
 **/
int SimpleRedisClient::wright_select(int fd, int timeout ) const
{
    struct timeval tv;
    fd_set fds;

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000)*1000;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    return select(fd+1, NULL, &fds, NULL, &tv);
}

void SimpleRedisClient::LogLevel(int l)
{
    debug = l;
}

int SimpleRedisClient::LogLevel(void)
{
    return debug;
}

int SimpleRedisClient::getError(void)
{
    return last_error;
}


int SimpleRedisClient::redis_send(char recvtype, const char *format, ...)
{
    if(fd == 0)
    {
        redis_conect();
    }

    data = 0;
    data_size = 0;

    if(answer_multibulk != 0)
    {
        delete []answer_multibulk;
        //delete answer_multibulk;
    }

    multibulk_arg = -1;
    answer_multibulk = 0;

    va_list ap;
    va_start(ap, format);

    bzero(buffer,buffer_size);
    int  rc = vsnprintf(buffer, buffer_size, format, ap);
    va_end(ap);

    if( rc < 0 )
    {
        std::clog<<name<<"error format"<<std::endl;
        return RC_ERR_DATA_FORMAT;
    }

    if( rc >= buffer_size )
    {
        std::clog<<name<<"error buffer overflow"<<std::endl;
        return RC_ERR_BUFFER_OVERFLOW;; // Не хватило буфера
    }

    if(debug > 3  ) std::clog<<name<<"send: "<<buffer<<std::endl;

    rc = send_data(buffer);

    if (rc != (int) strlen(buffer))
    {
        redis_conect();
        rc = send_data(buffer);
    }

    if (rc != (int) strlen(buffer))
    {
        if (rc < 0)
        {
            std::clog<<name<<"error send"<<std::endl;
            return RC_ERR_SEND;
        }
        std::clog<<name<<"error timeout"<<std::endl;
        return RC_ERR_TIMEOUT;
    }


    bzero(buffer,buffer_size);
    rc = read_select(fd, timeout);

    if (rc > 0)
    {

        int offset = 0;
        do
        {
            rc = recv(fd, buffer + offset, buffer_size - offset, 0);

            if(rc < 0)
            {
                std::clog<<name<<"error recv"<<std::endl;
                return CR_ERR_RECV;
            }

            if(rc >= buffer_size - offset && buffer_size * 2 > max_buffer_size)
            {
                char nullbuf[1000];
                int r = 0;
                while( (r = recv(fd, nullbuf, 1000, 0)) >= 0)
                {
                    if(debug) std::clog<<name<<"read "<<r<<"b"<<std::endl;
                }

                last_error = RC_ERR_DATA_BUFFER_OVERFLOW;
                break;
            }
            else if(rc >= buffer_size - offset && buffer_size * 2 < max_buffer_size)
            {
                if(debug) Log::gdb("REDIS Удвоение размера буфера до %d[rc=%d, buffer_size=%d, offset=%d]",buffer_size *2, rc, buffer_size, offset);

                int last_buffer_size = buffer_size;
                char* tbuf = buffer;

                buffer_size *= 2;
                buffer = new char[buffer_size];

                delete buf;
                buf = new char[buffer_size];

                memcpy(buffer, tbuf, last_buffer_size);
                offset = last_buffer_size;
            }
            else
            {
                break;
            }

        }
        while(1);
        if(debug > 3) std::clog<<name<<"recv:"<<rc<<" buf:"<<buffer<<std::endl;

        char prefix = buffer[0];
        if (recvtype != RC_ANY && prefix != recvtype && prefix != RC_ERROR)
        {
            std::clog<<name<<"error protocol:"<<recvtype<<" buf:"<<buffer<<std::endl;
            return RC_ERR_PROTOCOL;
        }

        char *p;
        int len = 0;
        switch (prefix)
        {
        case RC_ERROR:
            std::clog<<name<<"error :"<<buffer<<std::endl;
            data = buffer;
            data_size = rc;
            return rc;
        case RC_INLINE:
            data_size = strlen(buffer+1)-2;
            data = buffer+1;
            data[data_size] = 0;
            return rc;
        case RC_INT:
            data = buffer+1;
            data_size = rc;
            return rc;
        case RC_BULK:
            p = buffer;
            p++;

            if(*p == '-')
            {
                data = 0;
                data_size = -1;
                return rc;
            }

            while(*p != '\r')
            {
                len = (len*10)+(*p - '0');
                p++;
            }

            /* Now p points at '\r', and the len is in bulk_len. */
            if(debug > 3) Log::gdb("%d\n", len);

            data = p+2;
            data_size = len;
            data[data_size] = 0;

            return rc;
        case RC_MULTIBULK:
            data = buffer;

            p = buffer;
            p++;
            int delta = 0;
            multibulk_arg =  read_int(p, &delta);
            if(multibulk_arg < 1)
            {
                data = 0;
                data_size = -1;
                if(debug > 5) Log::gdb("redis RC_MULTIBULK data_size = 0");
                return rc;
            }

            data_size = multibulk_arg;

            answer_multibulk = new char*[multibulk_arg];

            p+= delta + 3;

            for(int i =0; i< multibulk_arg; i++)
            {
                if( buffer_size - 10 < p - buffer)
                {
                    multibulk_arg = i-1;
                    data_size = i - 1;
                    if(debug) Log::gdb("redis Выход по приближению к концу буфера [p=%d|multibulk_arg=%d]", (int)(p - buffer), multibulk_arg);
                    last_error = RC_ERR_DATA_BUFFER_OVERFLOW;
                    return rc;
                }

                len = 0;
                while(*p != '\r')
                {
                    len = (len*10)+(*p - '0');
                    p++;
                }

                p+=2;
                answer_multibulk[i] = p;

                p+= len;

                if( buffer_size - 1 < p - buffer )
                {
                    multibulk_arg = i-1;
                    data_size = i - 1;
                    if(debug) Log::gdb("redis Выход по приближению к концу буфера [p=%d|multibulk_arg=%d]", (int)(p - buffer), multibulk_arg);
                    last_error = RC_ERR_DATA_BUFFER_OVERFLOW;
                    return rc;
                }


                *p = 0;
                p+= 3;
            }

            return rc;
        }

        return rc;
    }
    else if (rc == 0)
    {
        std::clog<<name<<"error: connection close"<<std::endl;
        return RC_ERR_CONECTION_CLOSE; // Соединение закрыто
    }
    else
    {
        std::clog<<name<<"error: no data"<<std::endl;
        return RC_ERR; // error
    }
}

/**
 * Отправляет данные
 * @param buf
 * @return
 */
int SimpleRedisClient::send_data( const char *buf ) const
{
    fd_set fds;
    struct timeval tv;
    int sent = 0;

    /* NOTE: On Linux, select() modifies timeout to reflect the amount
     * of time not slept, on other systems it is likely not the same */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000)*1000;

    int tosend = strlen(buf); // При отправке бинарных данных возможны баги.

    while (sent < tosend)
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        int rc = select(fd + 1, NULL, &fds, NULL, &tv);

        if (rc > 0)
        {
            rc = send(fd, buf + sent, tosend - sent, 0);
            if (rc < 0)
            {
                std::clog<<name<<"error: send error 1"<<std::endl;
                return -1;
            }
            sent += rc;
        }
        else if (rc == 0) /* timeout */
        {
            break;
        }
        else
        {
            std::clog<<name<<"error: send error 2"<<std::endl;
            return -1;
        }
    }

    return sent;
}

/**
 *  public:
 */

void SimpleRedisClient::setPort(int Port)
{
    port = Port;
}

void SimpleRedisClient::setHost(const char* Host)
{
    host = Host;
}

/**
 * Соединение с редисом.
 */
int SimpleRedisClient::redis_conect(const char* Host,int Port)
{
    setPort(Port);
    setHost(Host);
    return redis_conect();
}

int SimpleRedisClient::redis_conect(const char* Host,int Port, int TimeOut)
{
    setPort(Port);
    setHost(Host);
    setTimeout(TimeOut);
    return redis_conect();
}

int SimpleRedisClient::redis_conect()
{
    int rc;
    struct sockaddr_in sa;
    bzero(&sa, sizeof(sa));

    if(debug > 1) Log::gdb("redis: connect %s:%d", host.c_str(), port);

    if(fd)
    {
        close(fd);
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 || setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&yes, sizeof(yes)) == -1 || setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&yes, sizeof(yes)) == -1)
    {
        std::clog<<"redis("<<name<<"): error socket"<<std::endl;
    }

    struct addrinfo hints, *info = NULL;
    bzero(&hints, sizeof(hints));

    if (inet_aton(host.c_str(), &sa.sin_addr) == 0)
    {
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        int err = getaddrinfo(host.c_str(), NULL, &hints, &info);
        if (err)
        {
            std::clog<<"redis("<<name<<"): mgetaddrinfo error: "<<gai_strerror(err)<<std::endl;
            return -1;
        }

        memcpy(&sa.sin_addr.s_addr, &(info->ai_addr->sa_data[2]), sizeof(in_addr_t));
        freeaddrinfo(info);
    }

    int flags = fcntl(fd, F_GETFL);
    if ((rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK)) < 0)
    {
        std::clog<<"redis("<<name<<"): error setting socket non-blocking failed"<<std::endl;
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0)
    {
        if (errno != EINPROGRESS)
        {
            std::clog<<"redis("<<name<<"): error connect"<<std::endl;
            return RC_ERR;
        }

        if (wright_select(fd, timeout) > 0)
        {
            int err = 0;
            unsigned int len = sizeof(err);
            if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1 || err)
            {
                std::clog<<"redis("<<name<<"): error getsockopt"<<std::endl;
                return RC_ERR;
            }
        }
        else /* timeout or select error */
        {
                std::clog<<"redis("<<name<<"): error connect timeout"<<std::endl;
            return RC_ERR_TIMEOUT;
        }
    }
    if(debug >  RC_LOG_DEBUG) Log::gdb("open ok %d", fd);

    return fd;
}

char** SimpleRedisClient::getMultiBulkData()
{
    return answer_multibulk;
}

/**
 * Вернёт количество ответов. Или -1 если последняя операция вернула что либо кроме множества ответов.
 * @return
 */
int SimpleRedisClient::getMultiBulkDataAmount()
{
    return multibulk_arg;
}

/**
 * Работает только после запроса данных которые возвращаются как множество ответов
 * @param i Номер ответа в множестве отвкетов
 * @return Вернёт ноль в случаи ошибки или указатель на данные
 */
char* SimpleRedisClient::getData(int i) const
{
    if(multibulk_arg > i)
    {
        return answer_multibulk[i];
    }
    return 0;
}

/**
 * Возвращает все члены множества, сохранённого в указанном ключе. Эта команда - просто упрощённый синтаксис для SINTER.
 * SMEMBERS key
 * Время выполнения: O(N).
 * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-smembers
 * @see http://redis.io/commands/smembers
 * @param key
 * @return
 */
int SimpleRedisClient::smembers(const char *key)
{
    return redis_send(RC_MULTIBULK, "SMEMBERS %s\r\n", key);
}

int SimpleRedisClient::smembers_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_MULTIBULK, "SMEMBERS")
}

/**
 * Returns the set cardinality (number of elements) of the set stored at key.
 * @see http://redis.io/commands/scard
 * @param key
 * @return
 */
int SimpleRedisClient::scard(const char *key)
{
    return redis_send(RC_INT, "SCARD %s\r\n", key);
}

/**
 * Returns the set cardinality (number of elements) of the set stored at key.
 * @see http://redis.io/commands/scard
 * @param key
 * @return
 */
int SimpleRedisClient::scard_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "SCARD")
}

/**
 * Ни ключь ни значение не должны содержать "\r\n"
 * @param key
 * @param val
 * @return
 */
int SimpleRedisClient::set(const char *key, const char *val)
{
    return redis_send( RC_INLINE, "SET %s %s\r\n",   key, val);
}

int SimpleRedisClient::set_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INLINE, "SET")
}

SimpleRedisClient& SimpleRedisClient::operator=(const char *key_val)
{
    redis_send( RC_INLINE, "SET %s\r\n", key_val);
    return *this;
}


int SimpleRedisClient::setex(const char *key, const char *val, int seconds)
{
    return redis_send(RC_INLINE, "SETEX %s %d %s\r\n",key, seconds, val);
}

int SimpleRedisClient::setex_printf(int seconds, const char *key, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    bzero(buf, buffer_size);
    int  rc = vsnprintf(buf, buffer_size, format, ap);
    va_end(ap);

    if( rc >= buffer_size )
    {
        return RC_ERR_BUFFER_OVERFLOW;; // Не хватило буфера
    }

    if(rc <  0)
    {
        return RC_ERR_DATA_FORMAT;
    }

    return redis_send(RC_INLINE, "SETEX %s %d %s\r\n",key, seconds, buf);
}

int SimpleRedisClient::setex_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INLINE, "SETEX")
}


int SimpleRedisClient::get(const char *key)
{
    return redis_send( RC_BULK, "GET %s\r\n", key);
}


int SimpleRedisClient::get_printf( const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_BULK, "GET")
}

/**
 * Add the specified members to the set stored at key. Specified members that are already a member of this set are ignored. If key does not exist, a new set is created before adding the specified members.
 * An error is returned when the value stored at key is not a set.
 * @see http://redis.io/commands/sadd
 * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-sadd
 * @return
 */
int SimpleRedisClient::sadd(const char *key, const char *member)
{
    return redis_send(RC_INT, "SADD %s %s\r\n", key, member);
}

/**
 * Add the specified members to the set stored at key. Specified members that are already a member of this set are ignored. If key does not exist, a new set is created before adding the specified members.
 * An error is returned when the value stored at key is not a set.
 * @see http://redis.io/commands/sadd
 * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-sadd
 * @param format
 * @param ... Ключь Значение (через пробел, в значении нет пробелов)
 * @return
 */
int SimpleRedisClient::sadd_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "SADD")
}

/**
 * Remove the specified members from the set stored at key.
 * Specified members that are not a member of this set are ignored.
 * If key does not exist, it is treated as an empty set and this command returns 0.
 * An error is returned when the value stored at key is not a set.
 *
 * @see http://redis.io/commands/srem
 * @param key
 * @param member
 * @return
 */
int SimpleRedisClient::srem(const char *key, const char *member)
{
    return redis_send(RC_INT, "SREM %s %s\r\n", key, member);
}

/**
 * Remove the specified members from the set stored at key.
 * Specified members that are not a member of this set are ignored. If key does not exist, it is treated as an empty set and this command returns 0.
 * An error is returned when the value stored at key is not a set.
 * @see http://redis.io/commands/srem
 * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-srem
 * @param format
 * @param ... Ключь Значение (через пробел, в значении нет пробелов)
 * @return
 */
int SimpleRedisClient::srem_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "SREM")
}


char* SimpleRedisClient::operator[] (const char *key)
{
    redis_send( RC_BULK, "GET %s\r\n", key);
    return getData();
}

SimpleRedisClient::operator char* () const
{
    return getData();
}

/**
 * @todo ЗАДОКУМЕНТИРОВАТЬ
 * @return
 */
SimpleRedisClient::operator int () const
{
    if(data_size < 1)
    {
        Log::gdb("SimpleRedisClient::operator int (%u) \n", data_size);
        return data_size;
    }

    if(getData() == 0)
    {
        Log::gdb("SimpleRedisClient::operator int (%u) \n", data_size);
        return -1;
    }

    int d = 0;
    int r = read_int(getData(), &d);

    Log::gdb("SimpleRedisClient::operator int (%u|res=%d) \n", data_size, r);

    return r;
}

/**
 * @todo ЗАДОКУМЕНТИРОВАТЬ
 * @return
 */
SimpleRedisClient::operator long () const
{
    if(data_size < 1)
    {
        Log::gdb("SimpleRedisClient::operator long (%u) \n", data_size);
        return data_size;
    }

    if(getData() == 0)
    {
        Log::gdb("SimpleRedisClient::operator long (%u) \n", data_size);
        return -1;
    }

    int d = 0;
    int r = read_long(getData(), &d);

    Log::gdb("SimpleRedisClient::operator long (%u|res=%d) \n", data_size, r);

    return r;
}

int SimpleRedisClient::getset(const char *key, const char *set_val)
{
    return redis_send( RC_BULK, "GETSET %s %s\r\n",   key, set_val);
}

int SimpleRedisClient::getset_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_BULK, "GETSET")
}

int SimpleRedisClient::ping()
{
    return redis_send( RC_INLINE, "PING\r\n");
}

int SimpleRedisClient::echo(const char *message)
{
    return redis_send( RC_BULK, "ECHO %s\r\n", message);;
}

int SimpleRedisClient::quit()
{
    return redis_send( RC_INLINE, "QUIT\r\n");
}

int SimpleRedisClient::auth(const char *password)
{
    return redis_send( RC_INLINE, "AUTH %s\r\n", password);
}


int SimpleRedisClient::getRedisVersion()
{
    /* We can receive 2 version formats: x.yz and x.y.z, where x.yz was only used prior
     * first 1.1.0 release(?), e.g. stable releases 1.02 and 1.2.6 */
    /* TODO check returned error string, "-ERR operation not permitted", to detect if
     * server require password? */
    if (redis_send( RC_BULK, "INFO\r\n") == 0)
    {
        sscanf(buffer, "redis_version:%9d.%9d.%9d\r\n", &version_major, &version_minor, &version_patch);
        return version_major;
    }

    return 0;
}


int SimpleRedisClient::setnx(const char *key, const char *val)
{
    return redis_send( RC_INT, "SETNX %s %s\r\n",  key, val);
}

int SimpleRedisClient::setnx_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "SETNX")
}

int SimpleRedisClient::append(const char *key, const char *val)
{
    return redis_send(RC_INT, "APPEND %s %s\r\n",  key, val);
}

int SimpleRedisClient::append_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "APPEND")
}

int SimpleRedisClient::substr( const char *key, int start, int end)
{
    return redis_send(RC_BULK, "SUBSTR %s %d %d\r\n",   key, start, end);
}

int SimpleRedisClient::exists( const char *key)
{
    return redis_send( RC_INT, "EXISTS %s\r\n", key);
}

int SimpleRedisClient::exists_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "EXISTS")
}


/**
 * Время выполнения: O(1)
 * Удаление указанных ключей. Если переданный ключ не существует, операция для него не выполняется. Команда возвращает количество удалённых ключей.
 * @see http://pyha.ru/wiki/index.php?title=Redis:cmd-del
 * @see http://redis.io/commands/del
 * @param key
 * @return
 */
int SimpleRedisClient::del( const char *key)
{
    return redis_send( RC_INT, "DEL %s\r\n", key);
}

int SimpleRedisClient::del_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "DEL")
}

/**
 * Удаляет все найденые ключи, тоесть ищет их командой keys и удаляет каждый найденый ключь.
 * @param key
 * @return Вернёт -1 или количество удалёных ключей
 * @todo Реализовать более быстрый способ работы
 */
int SimpleRedisClient::delete_keys( const char *key)
{
    if(keys(key))
    {
        if(getDataSize() < 1 || getBufferSize() < 1)
        {
            return 0;
        }

        char **ptr_data = new char*[getDataSize()];

        // Выделяем память для buf_data
        char *buf_data = new char[ getBufferSize() ];
        memcpy(buf_data, getData(), getBufferSize());

        long int offset = buf_data - getData(); // Опасное дело!
        int num_keys = getDataSize();
        for(int i =0; i< num_keys; i++)
        {
            ptr_data[i] = getData(i) + offset;
        }

        for(int i =0; i< num_keys; i++)
        {
            Log::gdb("del[%d]:%s\n", i, ptr_data[i]);
            del(ptr_data[i]);
        }

        // [SimpleRedisClient.cpp:1171]: (error) Mismatching allocation and deallocation: buf_data
        // Очищаем buf_data
        delete[] buf_data;
        delete[] ptr_data;
        return num_keys;
    }

    return -1;
}

/**
 * Удаляет все найденые ключи, тоесть ищет их командой keys и удаляет каждый найденый ключь.
 * @param key
 * @return Вернёт -1 или количество удалёных ключей
 * @todo Реализовать более быстрый способ работы
 */
int SimpleRedisClient::delete_keys_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    bzero(buf, buffer_size);
    int  rc = vsnprintf(buf, buffer_size, format, ap);
    va_end(ap);
    if( rc >= buffer_size ) return RC_ERR_BUFFER_OVERFLOW;
    if(rc <  0) return RC_ERR_DATA_FORMAT;

    if(redis_send(RC_MULTIBULK, "KEYS %s\r\n", buf))
    {
        if(getDataSize() < 1 || getBufferSize() < 1)
        {
            return 0;
        }

        char **ptr_data = new char*[getDataSize()];

        // Выделяем память для buf_data
        char *buf_data = new char[ getBufferSize() ];
        memcpy(buf_data, getData(), getBufferSize());

        long int offset = buf_data - getData(); // Опасное дело!
        int num_keys = getDataSize();
        for(int i =0; i< num_keys; i++)
        {
            ptr_data[i] = getData(i) + offset;
        }


        for(int i =0; i< num_keys; i++)
        {
            Log::gdb("del[%d]:%s\n", i, ptr_data[i]);
            del(ptr_data[i]);
        }

        // [SimpleRedisClient.cpp:1221]: (error) Mismatching allocation and deallocation: buf_data
        // Очищаем buf_data
        delete[] buf_data;

        delete[] ptr_data;
        return num_keys;
    }
    return -1;
}


int SimpleRedisClient::type( const char *key)
{
    return redis_send( RC_INLINE, "TYPE %s\r\n", key);

    /*if (rc == 0) {
      char *t = rhnd->reply.line;
      if (!strcmp("string", t))
        rc = CREDIS_TYPE_STRING;
      else if (!strcmp("list", t))
        rc = CREDIS_TYPE_LIST;
      else if (!strcmp("set", t))
        rc = CREDIS_TYPE_SET;
      else
        rc = CREDIS_TYPE_NONE;
    }*/

}

int SimpleRedisClient::keys( const char *pattern)
{
    return redis_send(RC_MULTIBULK, "KEYS %s\r\n", pattern);
}

int SimpleRedisClient::keys_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_MULTIBULK, "KEYS")
}


int SimpleRedisClient::randomkey()
{
    return redis_send( RC_BULK, "RANDOMKEY\r\n");
}

int SimpleRedisClient::flushall(void)
{
    return redis_send( RC_INLINE, "FLUSHALL\r\n");
}

int SimpleRedisClient::rename( const char *key, const char *new_key_name)
{
    return redis_send( RC_INLINE, "RENAME %s %s\r\n",   key, new_key_name);
}

int SimpleRedisClient::rename_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INLINE, "RENAME")
}

int SimpleRedisClient::renamenx( const char *key, const char *new_key_name)
{
    return redis_send( RC_INT, "RENAMENX %s %s\r\n", key, new_key_name);
}

int SimpleRedisClient::renamenx_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "RENAMENX")
}

int SimpleRedisClient::dbsize()
{
    return redis_send( RC_INT, "DBSIZE\r\n");
}

int SimpleRedisClient::expire( const char *key, int secs)
{
    return redis_send( RC_INT, "EXPIRE %s %d\r\n", key, secs);
}

int SimpleRedisClient::expire_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "EXPIRE")
}


int SimpleRedisClient::ttl( const char *key)
{
    return redis_send( RC_INT, "TTL %s\r\n", key);
}

int SimpleRedisClient::ttl_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "TTL")
}

// Списки

int SimpleRedisClient::lpush(const char *key, const char *val)
{
    return redis_send( RC_INT, "LPUSH %s %s\r\n", key, val);
}

int SimpleRedisClient::lpush_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "LPUSH")
}

int SimpleRedisClient::rpush(const char *key, const char *val)
{
    return redis_send( RC_INT, "RPUSH %s %s\r\n", key, val);
}

int SimpleRedisClient::rpush_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "RPUSH")
}

/**
 * LTRIM mylist 1 -1
 */
int SimpleRedisClient::ltrim(const char *key, int start_pos, int count_elem)
{
    return redis_send( RC_INLINE, "LTRIM %s %d %d\r\n", key, start_pos, count_elem);
}

/**
 * LTRIM mylist 1 -1
 */
int SimpleRedisClient::ltrim_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INLINE, "LTRIM")
}

/**
 * Выборка с конца очереди (то что попало в очередь раньше других), если все сообщения добавлялись с rpush
 * @param key
 * @return
 */
int SimpleRedisClient::lpop(const char *key)
{
    return redis_send( RC_INT, "LPUSH %s\r\n", key);
}

/**
 * Выборка с начала очереди (то что попало в очередь позже других), если все сообщения добавлялись с rpush
 * @param key
 * @return
 */
int SimpleRedisClient::lpop_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_BULK, "LPOP")
}

int SimpleRedisClient::rpop(const char *key)
{
    return redis_send( RC_INT, "RPUSH %s\r\n", key);
}

int SimpleRedisClient::rpop_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_BULK, "RPOP")
}

int SimpleRedisClient::llen(const char *key)
{
    return redis_send( RC_INT, "LLEN %s\r\n", key);
}

int SimpleRedisClient::llen_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "LLEN")
}


int SimpleRedisClient::lrem(const char *key, int n,const char* val)
{
    return redis_send( RC_INT, "LLEN %s %d %s\r\n", key, n, val);
}

int SimpleRedisClient::lrem_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "LREM")
}


int SimpleRedisClient::lrange(const char *key, int start, int stop)
{
    return redis_send( RC_INT, "LRANGE %s %d %d\r\n", key, start, stop);
}

int SimpleRedisClient::lrange_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_MULTIBULK, "LRANGE")
}


int SimpleRedisClient::incr(const char *key)
{
    return redis_send( RC_INT, "INCR %s\r\n", key);
}

int SimpleRedisClient::incr_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "INCR")
}

int SimpleRedisClient::decr(const char *key)
{
    return redis_send( RC_INT, "DECR %s\r\n", key);
}

int SimpleRedisClient::decr_printf(const char *format, ...)
{
    REDIS_PRINTF_MACRO_CODE(RC_INT, "DECR")
}

int SimpleRedisClient::operator +=( const char *key)
{
    return redis_send( RC_INT, "%s %s\r\n",  "INCR" , key);
}

int SimpleRedisClient::operator -=( const char *key)
{
    return redis_send( RC_INT, "%s %s\r\n",  "DECR" , key);
}

/**
 *
 * @param TimeOut
 */
void SimpleRedisClient::setTimeout( int TimeOut)
{
    timeout = TimeOut;
}

/**
 * Закрывает соединение
 */
void SimpleRedisClient::redis_close()
{
    if(debug > RC_LOG_DEBUG) Log::gdb("close ok %d\n", fd);
    if(fd != 0 )
    {
        close(fd);
    }
}


SimpleRedisClient::operator bool () const
{
    return fd != 0;
}

int SimpleRedisClient::operator == (bool d)
{
    return (fd != 0) == d;
}


char* SimpleRedisClient::getData() const
{
    return data;
}

int SimpleRedisClient::getDataSize() const
{
    return data_size;
}

std::string SimpleRedisClient::getRange(const std::string &key)
{
    int i;
    std::string retList;

    redis_send( RC_MULTIBULK, "ZREVRANGEBYSCORE %s 0 -1\r\n", key.c_str());

    for(i = 0; i < multibulk_arg; i++)
    {
        if(i == 0)
        {
            retList = std::string(answer_multibulk[i]);
        }
        else
        {
            retList += "," + std::string(answer_multibulk[i]);
        }
    }
    return retList;
}

std::string SimpleRedisClient::get(const std::string &key)
{
    redis_send( RC_BULK, "GET %s\r\n", key.c_str());

    if(data)
    {
        return std::string(data);
    }
    else
    {
        return std::string();
    }
}

bool SimpleRedisClient::exists(const std::string &key)
{
    redis_send( RC_INT, "EXISTS %s\r\n", key.c_str());

    if(data && data[0] == '1')
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool SimpleRedisClient::zrange(const std::string &key, std::list<unsigned> &ret)
{
    int i;

    redis_send( RC_MULTIBULK, "ZRANGE %s 0 -1\r\n", key.c_str());

    for(i = 0; i < multibulk_arg; i++)
    {
        ret.push_back(strtol(answer_multibulk[i],NULL,10));
    }

    return i > 0 ? true : false;
}

