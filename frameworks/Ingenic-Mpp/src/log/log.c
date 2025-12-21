#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"


#ifndef LOGLEVEL
        #define LOGLEVEL DEBUG
#endif

static const char *s_loginfo[] = {
        [ERROR] = "ERROR",
        [WARN]  = "WARN",
        [INFO]  = "INFO",
        [DEBUG] = "DEBUG",
};

int log_print(char *tag, char *filename, int line, LogLevel loglevel, const char *fmt, ...)
{
        if (loglevel > LOGLEVEL) {
                return 0;
        }

        va_list arg_list;
        char buf[1024];
        memset(buf, 0, 1024);
        va_start(arg_list, fmt);
        vsnprintf(buf, 1024, fmt, arg_list);

        switch (loglevel) {
        case ERROR:
                printf("[%s] %s  %d %s\n", tag, s_loginfo[ERROR], line, buf);
                break;
        case WARN:
                printf("[%s] %s  %d %s\n", tag, s_loginfo[WARN], line, buf);
                break;
        case INFO:
                printf("[%s] %s  %d %s\n", tag, s_loginfo[INFO], line, buf);
                break;
        case DEBUG:
                printf("[%s] %s  %d %s\n", tag, s_loginfo[DEBUG], line, buf);
                break;
        default:
                return 0;
        }

        va_end(arg_list);

        return 0;
}














