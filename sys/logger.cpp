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
#include <sys/syscall.h>


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
    int64_t lastRotate = m_realRotate.load();
    if (m_filename.empty() || (now - timezone) / m_rotateInterval == (lastRotate - timezone) / m_rotateInterval) {
        return;
    }
    
    long old = m_realRotate.exchange(now);
    //如果realRotate的值是新的，那么返回。否则，获得了旧值，进行rotate。防止多个线程同时rotate
    if ((old - timezone) / m_rotateInterval == (lastRotate - timezone) / m_rotateInterval) {
        return;
    }
    
    struct tm ntm;
    localtime_r(&now, &ntm);
    char newname[4096];
    snprintf(newname, sizeof(newname), "%s.%d_%02d_%02d_%02d_%02d", 
        m_filename.c_str(), ntm.tm_year + 1900, ntm.tm_mon + 1, ntm.tm_mday, ntm.tm_hour, ntm.tm_min);
    const char *oldname = m_filename.c_str();
    int err = rename(oldname, newname);
    if (err == -1) {
        if(ENOENT != errno){
            fprintf(stderr, "rename logfile %s -> %s failed msg: %s\n", oldname, newname, 
                strerror(errno));
            m_realRotate.exchange(old);
            return;
        }
    }

    int fd = open(m_filename.c_str(), O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fd == -1) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n", m_filename.c_str(), 
            strerror(errno));
        m_realRotate.exchange(old);
        return;
    }

    err = dup2(fd, m_fd);
    if(err == -1){
        fprintf(stderr, "dup2 failed. %s\n", strerror(errno));
        close(fd);
        m_realRotate.exchange(old);
        return;
    }

    std::thread t([=]{
        usleep(200 * 1000); // 睡眠200ms，参考leveldb做法
        close(fd);
    });
    t.detach();
}

void Logger::logv(int level, const char *file, int line, const char *func, const char *fmt...) {
    static thread_local uint64_t tid = syscall(SYS_gettid);

    if (level > m_level) {
        return;
    }
    maybeRotate();
    char buffer[4 * 1024];
    char *p = buffer;
    char *limit = buffer + sizeof(buffer);

    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    const time_t seconds = now_tv.tv_sec;
    struct tm t;
    localtime_r(&seconds, &t);
    p += snprintf(p, limit - p, "%04d/%02d/%02d-%02d:%02d:%02d.%06d %lx %s %s:%d ", 
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
        static_cast<int>(now_tv.tv_usec), (long) tid, m_levelStrs[level], file, line);
    va_list args;
    va_start(args, fmt);
    p += vsnprintf(p, limit - p, fmt, args);
    va_end(args);
    p = std::min(p, limit - 2);
    // trim the ending \n
    // while (*--p == '\n') {
    // }
    *++p = '\n';
    *++p = '\0';
    int fd = m_fd == -1 ? 1 : m_fd;
    int err = ::write(fd, buffer, p - buffer);
    if (err != p - buffer) {
        fprintf(stderr, "write log file %s failed. written %d errmsg: %s\n", 
            m_filename.c_str(), err, strerror(errno));
    }
    if (level <= ERROR) {
        syslog(LOG_ERR, "%s", buffer + 27);
    }
    if (level == FATAL) {
        fprintf(stderr, "%s", buffer);
        // assert(0);
    }
}