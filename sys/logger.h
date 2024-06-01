#pragma once

#include <cstdio>
#include <atomic>
#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace deps{
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
    static void logv(int level, const char *fmt, ...);
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
}