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

void Log::log(int level, const char* fmt, ... )
{
    FMTPARCE syslog(level, "%s", buffer);
};

void Log::err(const char* fmt, ... )
{
    char buffer[BUFLEN];
    va_list args;

    va_start (args, fmt);
    vsprintf (buffer,fmt, args);
    va_end (args);

    syslog(LOG_ERR, "%s", buffer);
};

void Log::err(const std::string &mes)
{
    syslog(LOG_ERR, "%s", mes.c_str());
};

void Log::warn(const char* fmt, ... )
{
    FMTPARCE syslog(LOG_WARNING, "%s", buffer);
};

void Log::info(const char* fmt, ... )
{
    FMTPARCE syslog(LOG_INFO, "%s", buffer);
};

void Log::gdb(const char* fmt, ... )
{
#ifdef DEBUG
    FMTPARCE syslog(LOG_DEBUG, "[%ld] %s", pthread_self(), buffer);
#endif // DEBUG
};
