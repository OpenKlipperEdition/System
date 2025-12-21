#ifndef __HW_LOG_H__
#define __HW_LOG_H__

typedef enum {
        ERROR = 1,
        WARN  = 2,
        INFO  = 3,
        DEBUG = 4,
} LogLevel;

int log_print(char *tag, char *filename, int line, LogLevel loglevel, const char *fmt, ...);

#define IHAL_LOGE(TAG,fmt,...) log_print(TAG,__FILE__,__LINE__,ERROR,fmt,## __VA_ARGS__)
#define IHAL_LOGW(TAG,fmt,...) log_print(TAG,__FILE__,__LINE__,WARN,fmt,## __VA_ARGS__)
#define IHAL_LOGI(TAG,fmt,...) log_print(TAG,__FILE__,__LINE__,INFO,fmt,## __VA_ARGS__)
#define IHAL_LOGD(TAG,fmt,...) log_print(TAG,__FILE__,__LINE__,DEBUG,fmt,## __VA_ARGS__)

#endif // __LOG_H__
