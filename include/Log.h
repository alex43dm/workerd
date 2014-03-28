#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <pthread.h>

#include <string>

#define BUFLEN 2048
#define FMTPARCE  char buffer[1024];\
  va_list args;\
  va_start (args, fmt);\
  vsprintf (buffer,fmt, args);\
  va_end (args);
//  perror (buffer);

class Log
{
public:
    enum LogPriority
    {
        Emerg   = LOG_EMERG,   // system is unusable
        Alert   = LOG_ALERT,   // action must be taken immediately
        Crit    = LOG_CRIT,    // critical conditions
        Err     = LOG_ERR,     // error conditions
        Warning = LOG_WARNING, // warning conditions
        Notice  = LOG_NOTICE,  // normal, but significant, condition
        Info    = LOG_INFO,    // informational message
        Debug   = LOG_DEBUG    // debug-level message
    };

    explicit Log(int facility);
    virtual ~Log();
    static void log(int level, const char* fmt, ... );
    static void err(const char* fmt, ... );
    static void err(const std::string &mes);
    static void warn(const char* fmt, ... );
    static void info(const char* fmt, ... );
    static void gdb(const char* fmt, ... );
private:
    int facility_;
    int priority_;
};

#endif // LOG_H
