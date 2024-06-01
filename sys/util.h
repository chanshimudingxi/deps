#pragma once

#include <string>
#include <stdint.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <iomanip>
#include <unistd.h>

namespace deps{

std::string DumpHex(const char* data, int size);

uint64_t GetMonoTimeUs();
uint64_t GetMonoTimeMs();
uint64_t GetLocalUtcTimeUs();
int64_t GetLocalUtcTimeMs();
std::string Utc2LocalTime(uint64_t utcTime);
uint64_t Utc2Ntp(uint64_t n);
uint64_t Ntp2Utc(uint64_t n);

std::string UintIP2String(uint32_t ip);

std::string getCWD();
}