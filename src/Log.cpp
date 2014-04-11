#include "Log.h"

extern char *__progname;

Log::Log(int facility)
{
    facility_ = facility;
    priority_ = LOG_DEBUG;
    openlog(__progname, LOG_PID, facility_);
}

Log::~Log()
{
    closelog();
}

void Log::err(const char* fmt, ... )
{
    va_list args;
    int len = vsnprintf( NULL, 0, fmt, args );
    char* buffer = new char[len];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_ERR, "%s", buffer);
    delete buffer;
};

void Log::err(const std::string &mes)
{
    syslog(LOG_ERR, "%s", mes.c_str());
};

void Log::warn(const char* fmt, ... )
{
    va_list args;
    int len = vsnprintf( NULL, 0, fmt, args );
    char* buffer = new char[len];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_WARNING, "%s", buffer);
    delete buffer;
};

void Log::info(const char* fmt, ... )
{
    va_list args;
    int len = vsnprintf( NULL, 0, fmt, args );
    char* buffer = new char[len];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_INFO, "%s", buffer);
    delete buffer;
};

void Log::gdb(const char* fmt, ... )
{
#ifdef DEBUG
    va_list args;
    int len = vsnprintf( NULL, 0, fmt, args );
    char* buffer = new char[len];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_DEBUG, "%s", buffer);
    delete buffer;
#endif // DEBUG
};
