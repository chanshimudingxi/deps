#ifndef UTIL_H_
#define UITL_H_
#include <string>
#include <stdint.h>
#include<time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Util{
public:
	static std::string DumpHex(const char* data, int size);
	static uint64_t GetMonoTimeUs();
	static uint64_t GetMonoTimeMs();
	std::string UintIP2String(uint32_t ip);
};


#endif