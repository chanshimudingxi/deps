#include "log.h"

int g_log_level=LOG_LEVEL_INFO;

void setLogLevel(int logLovel){
	g_log_level = logLovel;
}

void log(int priority, const char *fmt, ...){
	if(priority < g_log_level){
		return;
	}

    static char buf[1024*1024];
    buf[0] = '\0';
    va_list     param;
    va_start(param, fmt);
    int len = vsnprintf(buf, sizeof(buf) - 1, fmt, param);
    va_end(param);
    buf[len] = '\0';

	printf("%s",buf);
}
