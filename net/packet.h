#pragma once

#include <stdint.h>

#include "marshall.h"

namespace deps{
/**
 * 注意1：协议包总大小不能超过64K
 */
#define MAX_PACKET_SIZE 65535
#define MAIN_PROTO_HEADER_SIZE 8
#define SUB_PROTO_HEADER_SIZE 4

enum MainProtoType{
    MainProtoType_Unknown = 0,
    MainProtoType_Paxos = 1,
};

//主协议包头
struct ProtoHeader
{
    uint16_t m_length;
    uint16_t m_cmd;
    uint32_t m_seq;
};

//子协议包头
struct SubProtoHeader
{
    uint16_t m_cmd;             //子协议号
    uint16_t m_checkCode;       //校验码
};

//message消息格式
struct PacketHeader
{
	ProtoHeader m_mainProtoHeader; //消息头
    SubProtoHeader m_subProtoHeader; //stream_proto协议包头

    PacketHeader()
    {
        clear();
    }

    PacketHeader(const PacketHeader& header){
        m_mainProtoHeader.m_length = header.m_mainProtoHeader.m_length;
        m_mainProtoHeader.m_cmd = header.m_mainProtoHeader.m_cmd;
        m_mainProtoHeader.m_seq = header.m_mainProtoHeader.m_seq;
        m_subProtoHeader.m_cmd = header.m_subProtoHeader.m_cmd;
        m_subProtoHeader.m_checkCode = header.m_subProtoHeader.m_checkCode;
    }
    PacketHeader& operator= (const PacketHeader& header){
        m_mainProtoHeader.m_length = header.m_mainProtoHeader.m_length;
        m_mainProtoHeader.m_cmd = header.m_mainProtoHeader.m_cmd;
        m_mainProtoHeader.m_seq = header.m_mainProtoHeader.m_seq;
        m_subProtoHeader.m_cmd = header.m_subProtoHeader.m_cmd;
        m_subProtoHeader.m_checkCode = header.m_subProtoHeader.m_checkCode;
        return *this;
    }

    virtual ~PacketHeader(){}

    inline void clear(){
        m_mainProtoHeader.m_length = MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE;
        m_mainProtoHeader.m_cmd = MainProtoType_Paxos;
        m_mainProtoHeader.m_seq = 0;
        m_subProtoHeader.m_cmd = 0;
        m_subProtoHeader.m_checkCode = 0;
    }

    inline uint16_t getMainCmd() const 
    {
        return m_mainProtoHeader.m_cmd;
    }

    inline void setMainCmd(uint16_t cmd)
    {
        m_mainProtoHeader.m_cmd = cmd;
    }

    inline uint32_t getSeq() const
    {
        return m_mainProtoHeader.m_seq;
    }

    inline void setSeq(uint32_t seq)
    {
        m_mainProtoHeader.m_seq = seq;
    }

    inline uint16_t getLength() const
    {
        return m_mainProtoHeader.m_length;
    }

    inline void setLength(uint16_t len)
    {
        m_mainProtoHeader.m_length = len;
    }

    inline uint16_t getSubCmd() const 
    {
        return m_subProtoHeader.m_cmd;
    }

    inline void setSubCmd(uint16_t cmd) 
    {
        m_subProtoHeader.m_cmd = cmd;
    }

    inline uint16_t getCheckCode() const
    {
        return m_subProtoHeader.m_checkCode;
    }

    inline void setCheckCode(uint16_t checkcode)
    {
        m_subProtoHeader.m_checkCode = checkcode;
    }
};


//打包类
struct Encoder
{
public:
    Encoder():m_packetHeader(), pb(), headPack(pb, 0), bodyPack(pb, MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE){}
    
    Encoder(const Encoder &s):m_packetHeader(), pb(),headPack(pb, 0), bodyPack(pb, MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE)
    {
        m_packetHeader = s.m_packetHeader;
        headPack.replace(0, s.headPack.data(), MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE);
        bodyPack.push(s.bodyPack.data(), s.bodyPack.size());
    }

    virtual ~Encoder(){}
    
    const char* data()
    {
        return pb.data();
    }

    size_t size()
    {
        return MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE + bodySize();
    }

    const char* body()
    {
        return bodyPack.data();
    }

    size_t bodySize()
    {
        return bodyPack.size();
    }

    void clear()
    {
        pb.resize(0 + MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE);
        m_packetHeader.clear();
    }

    PacketHeader& getHeader(){
        return m_packetHeader;
    }
	
    void setSeq(uint32_t seq){
        m_packetHeader.setSeq(seq);
    }

    void serialize(uint16_t subCmd, const Marshallable &m, uint16_t (*check_code_func)(char* data, size_t size) = nullptr)
    {
        m.marshal(bodyPack);
        uint16_t checkcode = check_code_func ? check_code_func(bodyPack.data(), bodyPack.size()) : 0;
        m_packetHeader.setLength(size());
        m_packetHeader.setSubCmd(subCmd);
        m_packetHeader.setCheckCode(checkcode);
        headPack.replace_uint16(0, m_packetHeader.getLength());
        headPack.replace_uint16(2, m_packetHeader.getMainCmd());
        headPack.replace_uint32(4, m_packetHeader.getSeq());
        headPack.replace_uint16(8, m_packetHeader.getSubCmd());
        headPack.replace_uint16(10, m_packetHeader.getCheckCode());
    }
private:
    PacketHeader m_packetHeader;
	PackBuffer pb;
	Pack headPack;
	Pack bodyPack;
};

//解包类
class Decoder
{
public:
    static size_t minSize(){return MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE;}
    static size_t maxSize(){return MAX_PACKET_SIZE;}
    static size_t mainHeaderSize(){return MAIN_PROTO_HEADER_SIZE;}
    static size_t headerSize(){return MAIN_PROTO_HEADER_SIZE + SUB_PROTO_HEADER_SIZE;}
    static uint16_t pickLen(const void * data){uint16_t i = *((uint16_t*)data);return XHTONS(i);};
    static uint16_t pickCmd(const void * data){uint16_t i = *((uint16_t*)((char*)data+2));return XHTONS(i);};
    static uint32_t pickSeq(const char *data){uint32_t i = *((uint32_t*)((char*)data+4)); return XHTONL(i);};
    static uint16_t pickSubCmd(const void * data){uint16_t i = *((uint16_t*)((char*)data+8));return XHTONS(i);};
    static uint16_t pickCheckCode(const char *data){uint16_t i = *((uint16_t*)((char*)data+10)); return XHTONS(i);};

    Decoder(const char *data, uint16_t sz):up(data, sz){};
    virtual ~Decoder(){};

    void deserialize(PacketHeader& packetHeader, Marshallable& msg)
    {
        packetHeader.setLength(up.pop_uint16());
        packetHeader.setMainCmd(up.pop_uint16());
        packetHeader.setSeq(up.pop_uint32());
        packetHeader.setSubCmd(up.pop_uint16());
        packetHeader.setCheckCode(up.pop_uint16());
        msg.unmarshal(up);
    }
private:
	Unpack up;
};

}