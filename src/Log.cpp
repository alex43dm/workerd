#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ostream>

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
    char* buffer = new char[BUFLEN];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_ERR, "%s", buffer);
    delete []buffer;
};

void Log::err(const std::string &mes)
{
    syslog(LOG_ERR, "%s", mes.c_str());
};

void Log::warn(const char* fmt, ... )
{
    va_list args;
    char* buffer = new char[BUFLEN];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_WARNING, "%s", buffer);
    delete []buffer;
};

void Log::info(const char* fmt, ... )
{
    va_list args;
    char* buffer = new char[BUFLEN];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_INFO, "%s", buffer);
    delete []buffer;
};

void Log::gdb(const char* fmt, ... )
{
#ifdef DEBUG
    va_list args;
    char* buffer = new char[BUFLEN];

    va_start (args, fmt);
    vsprintf(buffer, fmt, args);
    va_end (args);

    syslog(LOG_DEBUG, "%s", buffer);
    delete []buffer;
#endif // DEBUG
};

int Log::sync()
{
    if (buffer_.length())
    {
        syslog(priority_, "%s",buffer_.c_str());
        buffer_.erase();
        //priority_ = LOG_DEBUG; // default to debug for each message
    }
    return 0;
}

int Log::overflow(int c)
{
    if (c != EOF)
    {
        buffer_ += static_cast<char>(c);
    }
    else
    {
        sync();
    }
    return c;
}

std::ostream& operator<< (std::ostream& os, const LogPriority& log_priority)
{
    static_cast<Log *>(os.rdbuf())->priority_ = (int)log_priority;
    return os;
}

int Log::parseLine(char* line)
{
    int i = strlen(line);
    while (*line < '0' || *line > '9') line++;
    line[i-3] = '\0';
    i = atoi(line);
    return i;
}


int Log::memUsage()
{
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];


    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmSize:", 7) == 0)
        {
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}

static unsigned long long proc_times,total_cpu_usage;

float Log::cpuUsage()
{
    FILE* file = fopen("/proc/self/stat", "r");
    char line[1024];
    unsigned long long user,nice,system,idle;
    float ret;

    fgets(line, sizeof(line), file);
    sscanf(line,"%*s %llu %llu %llu %llu",&user,&nice,&system,&idle);
    fclose(file);

    ret = (float)(user + nice + system + idle - total_cpu_usage);
    if(ret != 0)
    {
        ret = sysconf(_SC_NPROCESSORS_ONLN)*(user + system - proc_times)*100/ret;
    }

    proc_times = user + system;
    total_cpu_usage = user + nice + system + idle;

    return ret;
}

