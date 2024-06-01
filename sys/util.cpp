#include "util.h"
#include <unistd.h>

using namespace deps;

std::string Util::DumpHex(const char* data,int size){
    if(nullptr == data || size < 1){
        return "";
    }

    std::stringstream ss;
    for(int i = 0; i < size; ++i){
        ss<<"0x" <<std::setw(2) <<std::setfill('0') <<std::hex <<(data[i]&0xFF) <<" ";
        if(i < size && i % 8 == 7){
            ss<<'\n';
        }
    }

    return ss.str();
}

uint64_t Util::GetMonoTimeUs(){
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t utime = (uint64_t)time.tv_sec * 1000000;
	utime += time.tv_nsec/1000;

	return  utime;
}

uint64_t Util::GetMonoTimeMs(){
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t utime = (uint64_t)time.tv_sec * 1000;
	utime += time.tv_nsec/1000000;

	return  utime;
}

uint64_t Util::GetLocalUtcTimeUs(){
	struct timeval now_tv;
	gettimeofday(&now_tv, NULL);
	uint64_t utcNow = now_tv.tv_sec * 1000000;
	utcNow += now_tv.tv_usec;
	return utcNow;
}


int64_t Util::GetLocalUtcTimeMs(){
    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    int64_t utcNow = now_tv.tv_sec * 1000;
    utcNow += now_tv.tv_usec/1000;
    return utcNow;
}

std::string Util::Utc2LocalTime(uint64_t utcTime)
{
	time_t sec = (time_t)(utcTime / 1000000);
	uint32_t us = (uint32_t)(utcTime % 1000000);
    struct tm t;
    localtime_r(&sec, &t);
	char str[64];
    snprintf(str, sizeof(str), "%04d/%02d/%02d-%02d:%02d:%02d.%06u", 
        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, us);
	return std::string(str);
}

uint64_t Util::Utc2Ntp(uint64_t n)
{
    static const uint32_t SECOND_70_YEAR = (uint32_t)60 * (uint32_t)60 * (uint32_t)24 * 
        ((uint32_t)365 * (uint32_t)70 + (uint32_t)17);
    uint64_t H32bit, L32bit;
    uint64_t ntpTime;
    
    H32bit = n / 1000000 + SECOND_70_YEAR;
    L32bit = (((n % 1000000) << 32) / 1000000);
    ntpTime = H32bit << 32 | L32bit;
    return ntpTime;
}

uint64_t Util::Ntp2Utc(uint64_t n)
{
    static const uint32_t SECOND_70_YEAR = (uint32_t)60 * (uint32_t)60 * (uint32_t)24 * 
        ((uint32_t)365 * (uint32_t)70 + (uint32_t)17);
    uint64_t H32bit, L32bit;
    uint64_t utcTime;
 
    H32bit = (n >> 32) - SECOND_70_YEAR;
    L32bit = n & 0x00000000ffffffff;
    L32bit = (L32bit * 1000000) >> 32;
    utcTime = H32bit * 1000000 + L32bit;
    return utcTime;
}

std::string Util::UintIP2String(uint32_t ip){
	struct in_addr saddr;
	saddr.s_addr = ip;
	return std::string(inet_ntoa(saddr));
}


std::string Util::getCWD()
{
	char* cwd = getcwd(NULL, 0);
	std::string ret = ".";
	if (nullptr != cwd)
	{
		ret.assign(cwd);
		free(cwd);
	}

	return ret;
}
