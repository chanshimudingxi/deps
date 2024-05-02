#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <atomic>
#include <string>

#define log(level, ...)                                                                \
    do {                                                                                \
        if (level <= Logger::getLogger().getLogLevel()) {                               \
            snprintf(0, 0, __VA_ARGS__);                                                \
            Logger::getLogger().logv(level, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        }                                                                               \
    } while (0)

#define LOG_TRACE(...) log(Logger::TRACE, __VA_ARGS__)
#define LOG_DEBUG(...) log(Logger::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log(Logger::INFO, __VA_ARGS__)
#define LOG_WARN(...) log(Logger::WARN, __VA_ARGS__)
#define LOG_ERROR(...) log(Logger::ERROR, __VA_ARGS__)
#define LOG_FATAL(...) log(Logger::FATAL, __VA_ARGS__)

#define setloglevel(l) Logger::getLogger().setLogLevel(l)
#define setlogfile(n) Logger::getLogger().setFileName(n)

class Logger{
public:
    enum LogLevel { FATAL = 0, ERROR, UERR, WARN, INFO, DEBUG, TRACE, ALL };
    Logger();
    Logger (const Logger&) = delete;
    Logger& operator= (const Logger&) = delete;
    ~Logger();
    void logv(int level, const char *file, int line, const char *func, const char *fmt...);

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