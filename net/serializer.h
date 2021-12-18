#ifndef SERIALIZER_H_
#define SERIALIZER_H_

#include <string>
#include <stdint.h>
#include <cstddef>

class Serializer{
public:
    static void PutUint8(uint8_t data, std::string* s);
    static void PutUint16(uint16_t data, std::string* s);
    static void PutUint32(uint32_t data, std::string* s);
    static void PutUint64(uint64_t data, std::string* s);
    static void PutFloat(float data, std::string* s);
    static void PutDouble(double data, std::string* s);
    static void PutString(const std::string& data, std::string* s);

    static bool GetUint8(const char* buffer, size_t size, uint8_t* value);
    static bool GetUint16(const char* buffer, size_t size, uint16_t* value);
    static bool GetUint32(const char* buffer, size_t size, uint32_t* value);
    static bool GetUint64(const char* buffer, size_t size, uint64_t* value);
    static bool GetFloat(const char* buffer, size_t size, float* value);
    static bool GetDouble(const char* buffer, size_t size, double* value);
    static bool GetString(const char* buffer, size_t size, std::string* value);
};

#endif
