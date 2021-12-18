#include "serializer.h"

void Serializer::PutUint8(uint8_t data, std::string* s){
    s->append((char*)(&data), 1);
}

void Serializer::PutUint16(uint16_t data, std::string* s){
    uint8_t value[2];
    value[0] = (uint8_t)(data & 0x00FF);
    value[1] = (uint8_t)(data >> 8 & 0x00FF); 
    s->append((char*)value, 2);
}

void Serializer::PutUint32(uint32_t data, std::string* s){
    uint8_t value[4];
    value[0] = (uint8_t)(data & 0x000000FF);
    value[1] = (uint8_t)(data >> 8 & 0x000000FF); 
    value[2] = (uint8_t)(data >> 16 & 0x000000FF);
    value[3] = (uint8_t)(data >> 24 & 0x000000FF); 

    s->append((char*)value, 4);
}

void Serializer::PutUint64(uint64_t data, std::string* s){
    uint8_t value[8];
    value[0] = (uint8_t)(data & 0x00000000000000FF);
    value[1] = (uint8_t)(data >> 8 & 0x00000000000000FF); 
    value[2] = (uint8_t)(data >> 16 & 0x00000000000000FF);
    value[3] = (uint8_t)(data >> 24 & 0x00000000000000FF); 
    value[4] = (uint8_t)(data >> 32 & 0x00000000000000FF); 
    value[5] = (uint8_t)(data >> 40 & 0x00000000000000FF);
    value[6] = (uint8_t)(data >> 48 & 0x00000000000000FF); 
    value[7] = (uint8_t)(data >> 56 & 0x00000000000000FF);

    s->append((char*)value, 8);
}

void Serializer::PutFloat(float data, std::string* s){
    uint32_t* p = (uint32_t*)(&data);
    uint8_t value[4];
    value[0] = (uint8_t)(*p & 0x000000FF);
    value[1] = (uint8_t)(*p >> 8 & 0x000000FF); 
    value[2] = (uint8_t)(*p >> 16 & 0x000000FF);
    value[3] = (uint8_t)(*p >> 24 & 0x000000FF); 

    s->append((char*)value, 4);
}

void Serializer::PutDouble(double data, std::string* s){
    uint64_t* p = (uint64_t*)(&data);
    uint8_t value[8];
    value[0] = (uint8_t)(*p & 0x00000000000000FF);
    value[1] = (uint8_t)(*p >> 8 & 0x00000000000000FF); 
    value[2] = (uint8_t)(*p >> 16 & 0x00000000000000FF);
    value[3] = (uint8_t)(*p >> 24 & 0x00000000000000FF); 
    value[4] = (uint8_t)(*p >> 32 & 0x00000000000000FF); 
    value[5] = (uint8_t)(*p >> 40 & 0x00000000000000FF);
    value[6] = (uint8_t)(*p >> 48 & 0x00000000000000FF); 
    value[7] = (uint8_t)(*p >> 56 & 0x00000000000000FF);

    s->append((char*)value, 8);
}

void Serializer::PutString(const std::string& data, std::string* s){
    uint32_t size = data.size();
    PutUint32(size,s);
    s->append(data.data(),data.size());
}

bool Serializer::GetUint8(const char* buffer, size_t size, uint8_t* value){
    if(size < 1 || buffer == nullptr){
        return false;
    }
    *value = (uint8_t)buffer[0];
    return true;
}

bool Serializer::GetUint16(const char* buffer, size_t size, uint16_t* value){
    if(size < 2 || buffer == nullptr){
        return false;
    }
    *value = ((uint16_t)buffer[0])        & 0x00FF;
    *value |= ((uint16_t)buffer[1] << 8)  & 0xFF00; 
    return true;
}

bool Serializer::GetUint32(const char* buffer, size_t size, uint32_t* value){
    if(size < 4 || buffer == nullptr){
        return false;
    }
    *value = ((uint32_t)buffer[0])        & 0x000000FF;
    *value |= ((uint32_t)buffer[1] << 8)  & 0x0000FF00; 
    *value |= ((uint32_t)buffer[2] << 16) & 0x00FF0000; 
    *value |= ((uint32_t)buffer[3] << 24) & 0xFF000000; 
    return true;
}

bool Serializer::GetUint64(const char* buffer, size_t size, uint64_t* value){
    if(size < 8 || buffer == nullptr){
        return false;
    }
    *value = ((uint64_t)buffer[0])        & 0x00000000000000FF;
    *value |= ((uint64_t)buffer[1] << 8)  & 0x000000000000FF00; 
    *value |= ((uint64_t)buffer[2] << 16) & 0x0000000000FF0000; 
    *value |= ((uint64_t)buffer[3] << 24) & 0x00000000FF000000; 
    *value |= ((uint64_t)buffer[4] << 32) & 0x000000FF00000000; 
    *value |= ((uint64_t)buffer[5] << 40) & 0x0000FF0000000000; 
    *value |= ((uint64_t)buffer[6] << 48) & 0x00FF000000000000; 
    *value |= ((uint64_t)buffer[7] << 56) & 0xFF00000000000000; 
    return true;
}

bool Serializer::GetFloat(const char* buffer, size_t size, float* value){
    if(size < 4 || buffer == nullptr){
        return false;
    }
    uint32_t u32Value;
    u32Value = ((uint32_t)buffer[0])        & 0x000000FF;
    u32Value |= ((uint32_t)buffer[1] << 8)  & 0x0000FF00; 
    u32Value |= ((uint32_t)buffer[2] << 16) & 0x00FF0000; 
    u32Value |= ((uint32_t)buffer[3] << 24) & 0xFF000000;
    *value  = *((float*)&u32Value);
    return true;
}

bool Serializer::GetDouble(const char* buffer, size_t size, double* value){
    if(size < 8 || buffer == nullptr){
        return false;
    }
    uint64_t u64Value;
    u64Value = ((uint64_t)buffer[0])        & 0x00000000000000FF;
    u64Value |= ((uint64_t)buffer[1] << 8)  & 0x000000000000FF00; 
    u64Value |= ((uint64_t)buffer[2] << 16) & 0x0000000000FF0000; 
    u64Value |= ((uint64_t)buffer[3] << 24) & 0x00000000FF000000;
    u64Value |= ((uint64_t)buffer[4] << 32) & 0x000000FF00000000;
    u64Value |= ((uint64_t)buffer[5] << 40) & 0x0000FF0000000000;
    u64Value |= ((uint64_t)buffer[6] << 48) & 0x00FF000000000000;
    u64Value |= ((uint64_t)buffer[7] << 56) & 0xFF00000000000000;

    *value  = *((double*)&u64Value);
    return true;
}

bool Serializer::GetString(const char* buffer, size_t size, std::string* value){
    uint32_t u32Size = 0;
    if(!GetUint32(buffer, size, &u32Size)){
		return false;
    }
	if(size < u32Size + 4){
		return false;
	}
	value->append(buffer + 4, u32Size);
	return true;
}