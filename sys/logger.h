#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <atomic>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#ifdef __APPLE__
#define gettid() syscall(SYS_thread_selfid)
#else
#define gettid() syscall(SYS_gettid)
#endif

#define log(level, fmt, ...)                                \
    do {                                                    \
        if (level <= Logger::getLogger().getLogLevel()) {   \
            struct timeval now_tv;                          \
            gettimeofday(&now_tv, NULL);                    \
            time_t seconds = now_tv.tv_sec;                 \
            struct tm t;                                    \
            localtime_r(&seconds, &t);                      \
            static thread_local uint64_t tid = gettid();    \
            snprintf(0, 0, "[%04d/%02d/%02d-%02d:%02d:%02d.%06d](%lu)[%s:%d::%s]" fmt,          \
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,       \
                (int)(now_tv.tv_usec), tid, __FILE__, __LINE__, __func__, ##__VA_ARGS__);       \
            logger_vlog(level, "[%04d/%02d/%02d-%02d:%02d:%02d.%06d](%lu)[%s:%d::%s]" fmt,      \
                t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,       \
                (int)(now_tv.tv_usec), tid, __FILE__, __LINE__, __func__, ##__VA_ARGS__);       \
        }                                                                                       \
    } while (0)

#define LOG_TRACE(fmt, ...) log(Logger::TRACE, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log(Logger::DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) log(Logger::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) log(Logger::WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log(Logger::ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log(Logger::FATAL, fmt, ##__VA_ARGS__)

#define setloglevel(l) Logger::getLogger().setLogLevel(l)
#define setlogfile(n) Logger::getLogger().setFileName(n)

extern void logger_vlog(int level, const char *fmt, ...);

class Logger{
public:
    enum LogLevel { FATAL = 0, ERROR, UERR, WARN, INFO, DEBUG, TRACE, ALL };
    Logger();
    Logger (const Logger&) = delete;
    Logger& operator= (const Logger&) = delete;
    ~Logger();
    void vlog(int level, const char *fmt, va_list args);
    int setFileName(const std::string &filename);
    void setLogLevel(const std::string &level);
    void setLogLevel(LogLevel level) { m_level = std::min(ALL, std::max(FATAL, level)); }

    inline LogLevel getLogLevel() { return m_level; }
    const char *getLogLevelStr() { return m_levelStrs[m_level]; }
    inline int getFd() { return m_fd; }

    void adjustLogLevel(int adjust) { setLogLevel(LogLevel(m_level + adjust)); }
    void setRotateInterval(long rotateInterval) { m_rotateInterval = rotateInterval; }
    static Logger &getLogger();

private:
    void maybeRotate();
private:
    static const char *m_levelStrs[ALL + 1];
    int m_fd;
    LogLevel m_level;
    std::atomic<int64_t> m_realRotate;
    long m_rotateInterval;
    std::string m_filename;
};

#endif // LOGGER_H