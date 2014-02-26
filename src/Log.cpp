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
    FMTPARCE syslog(LOG_ERR, "%s", buffer);
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
