#include "logger.h"
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <cstdarg>
#include <cstring>
#include <sys/stat.h>
#include <ctime>
#include <syslog.h>
#include <unistd.h>
#include <thread>
#include <sys/types.h>
#include <sys/time.h>

using namespace deps;

Logger::Logger() : m_level(INFO), m_rotateInterval(86400) {
    tzset();
    m_fd = -1;
    m_realRotate = time(NULL);
}

Logger::~Logger() {
    if (m_fd != -1) {
        close(m_fd);
    }
}

const char *Logger::m_levelStrs[ALL + 1] = {
    "FATAL", "ERROR", "UERR", "WARN", "INFO", "DEBUG", "TRACE", "ALL",
};

Logger &Logger::getLogger() {
    static Logger logger;
    return logger;
}

void Logger::setLogLevel(const std::string &level) {
    LogLevel ilevel = INFO;
    for (size_t i = 0; i < sizeof(m_levelStrs) / sizeof(const char *); i++) {
        if (strcasecmp(m_levelStrs[i], level.c_str()) == 0) {
            ilevel = (LogLevel) i;
            setLogLevel(ilevel);
            return;
        }
    }
}

int Logger::setFileName(const std::string &filename) {
    int fd = open(filename.c_str(), O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fd < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n", filename.c_str(), strerror(errno));
        return -1;
    }
    m_filename = filename;
    if (m_fd == -1) {
        m_fd = fd;
    } else {
        int r = dup2(fd, m_fd);
        if(r < 0){
            fprintf(stderr, "dup2 failed. %s\n", strerror(errno));
        }
        close(fd);
    }
    return 0;
}

void Logger::maybeRotate() {
    time_t now = time(NULL);
    long lastRotate = m_realRotate.load();
    if (m_filename.empty() || (now - timezone) / m_rotateInterval == (lastRotate - timezone) / m_rotateInterval) {
        return;
    }
    
    long lastRotateBackup = m_realRotate.exchange(now);
    //如果realRotate的值是新的，那么返回。否则，获得了旧值，进行rotate。防止多个线程同时rotate
    if ((now - timezone) / m_rotateInterval == (lastRotateBackup - timezone) / m_rotateInterval) {
        return;
    }

    struct tm ntm;
    localtime_r(&now, &ntm);
    char newname[4096];
    snprintf(newname, sizeof(newname), "%s.%d_%02d_%02d_%02d_%02d_%02d", 
        m_filename.c_str(), ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday, ntm.tm_hour, ntm.tm_min, ntm.tm_sec);
    const char *oldname = m_filename.c_str();
    int err = rename(oldname, newname);
    if (err == -1) {
        if(ENOENT != errno){
            fprintf(stderr, "rename logfile %s -> %s failed msg: %s\n", oldname, newname, 
                strerror(errno));
            m_realRotate.exchange(lastRotateBackup);
            return;
        }
    }

    int fd = open(m_filename.c_str(), O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fd == -1) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n", m_filename.c_str(), 
            strerror(errno));
        m_realRotate.exchange(lastRotateBackup);
        return;
    }

    err = dup2(fd, m_fd);
    if(err == -1){
        fprintf(stderr, "dup2 failed. %s\n", strerror(errno));
        close(fd);
        m_realRotate.exchange(lastRotateBackup);
        return;
    }

    std::thread t([=]{
        usleep(200 * 1000); // 睡眠200ms，参考leveldb做法
        close(fd);
    });
    t.detach();
}

void Logger::vlog(int level, const char *fmt, va_list args){
    if (level > m_level) {
        return;
    }

    maybeRotate();

    char buffer[4096];
    char *p = buffer;
    char *limit = buffer + sizeof(buffer);

    p += snprintf(p, limit - p, "[%5s]", m_levelStrs[level]);

    p += vsnprintf(p, limit - p, fmt, args);

    p = std::min(p, limit - 2);
    *++p = '\n';
    *++p = '\0';

    int fd = m_fd == -1 ? 1 : m_fd;
    int err = ::write(fd, buffer, p - buffer);
    if (err != p - buffer) {
        fprintf(stderr, "write log file %s failed. written %d errmsg: %s\n", 
            m_filename.c_str(), err, strerror(errno));
    }
    //错误级别日志写入syslog
    if(level <= ERROR){
        syslog(LOG_ERR, "%s", buffer + 35);
    }
}

void logger_vlog(int level, const char *fmt, ...){
    if (level > Logger::getLogger().getLogLevel()) { 
        return;
    }

    va_list args;
    va_start(args, fmt);
    Logger::getLogger().vlog(level, fmt, args);
    va_end(args);
}