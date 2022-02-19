#include "util.h"
#include <sstream>
#include <iomanip>

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

std::string Util::UintIP2String(uint32_t ip){
	struct in_addr saddr;
	saddr.s_addr = ip;
	return std::string(inet_ntoa(saddr));
}
