#ifndef UTIL_H_
#define UITL_H_
#include <string>
#include <stdint.h>
#include<time.h>

class Util{
public:
	static std::string DumpHex(const char* data, int size);
	static uint64_t GetMonoTimeUs();
};


#endif