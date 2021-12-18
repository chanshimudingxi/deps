#ifndef LOG_H_
#define LOG_H_

#include <stdarg.h>
#include <stdio.h>


#define	LOG_LEVEL_DEBUG 0
#define	LOG_LEVEL_INFO 1
#define	LOG_LEVEL_WARN 2
#define	LOG_LEVEL_ERROR 3

extern int g_log_level;
extern void setLogLevel(int logLovel);
extern void log(int priority, const char *fmt, ...);

#define LOG_DEBUG(fmt, args...) log(LOG_LEVEL_DEBUG, "[DEBUG]%u:%s:%u(%s): " fmt "\n",   uint32_t(time(NULL)), __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG_INFO(fmt, args...)  log(LOG_LEVEL_INFO,  "[INFO]%u:%s:%u(%s): "  fmt "\n",   uint32_t(time(NULL)), __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG_WARN(fmt, args...)  log(LOG_LEVEL_WARN,  "[WARN]%u:%s:%u(%s): "  fmt "\n",   uint32_t(time(NULL)), __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG_ERROR(fmt, args...) log(LOG_LEVEL_ERROR, "[ERROR]%u:%s:%u(%s): " fmt "\n",   uint32_t(time(NULL)), __FILE__, __LINE__, __FUNCTION__, ##args)


#endif