#pragma once

#include "logger.h"

#define setloglevel(l) deps::Logger::getLogger().setLogLevel(l)
#define setlogfile(n) deps::Logger::getLogger().setFileName(n)

#ifdef __APPLE__
#define gettid() syscall(SYS_thread_selfid)
#else
#define gettid() syscall(SYS_gettid)
#endif

#define log(level, fmt, ...)                                \
    do {                                                    \
        if (level <= deps::Logger::getLogger().getLogLevel()) {   \
            struct timeval now_tv;                          \
            gettimeofday(&now_tv, NULL);                    \
            time_t seconds = now_tv.tv_sec;                 \
            struct tm t;                                    \
            localtime_r(&seconds, &t);                      \
            static thread_local uint64_t tid = gettid();    \
            snprintf(0, 0, "[%04d/%02d/%02d-%02d:%02d:%02d.%06d](%lu)[%s:%d::%s]" fmt,          \
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,       \
                (int)(now_tv.tv_usec), tid, __FILE__, __LINE__, __func__, ##__VA_ARGS__);       \
            deps::Logger::logv(level, "[%04d/%02d/%02d-%02d:%02d:%02d.%06d](%lu)[%s:%d::%s]" fmt,\
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,       \
                (int)(now_tv.tv_usec), tid, __FILE__, __LINE__, __func__, ##__VA_ARGS__);       \
        }                                                                                       \
    } while (0)

#define LOG_TRACE(fmt, ...) log(deps::Logger::TRACE, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log(deps::Logger::DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) log(deps::Logger::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) log(deps::Logger::WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log(deps::Logger::ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log(deps::Logger::FATAL, fmt, ##__VA_ARGS__)
