#pragma once

#include <stdint.h>

#include "marshall.h"

namespace deps{
#define RES_SUCCESS     0     //功能成功完成
#define RES_FAIL        1     //功能失败

/**
 * 注意1：协议包总大小不能超过64K
 */
#define MAX_PACKET_SIZE 65535
#define PROTO_HEADER_SIZE 8
#define PAXOS_PROTO_HEADER_SIZE 6

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
struct PaxosProtoHeader
{
    uint16_t m_length;
    uint16_t m_cmd;
    uint16_t m_resCode;
};

//message消息格式
struct PacketHeader
{
	ProtoHeader m_protoHeader; //消息头
    PaxosProtoHeader m_paxosProtoHeader; //stream_proto协议包头

    PacketHeader()
    {
        clear();
    }

    PacketHeader(const PacketHeader& header){
        m_protoHeader.m_length = header.m_protoHeader.m_length;
        m_protoHeader.m_cmd = header.m_protoHeader.m_cmd;
        m_protoHeader.m_seq = header.m_protoHeader.m_seq;
        m_paxosProtoHeader.m_cmd = header.m_paxosProtoHeader.m_cmd;
        m_paxosProtoHeader.m_length = header.m_paxosProtoHeader.m_length;
        m_paxosProtoHeader.m_resCode = header.m_paxosProtoHeader.m_resCode;
    }
    PacketHeader& operator= (const PacketHeader& header){
        m_protoHeader.m_length = header.m_protoHeader.m_length;
        m_protoHeader.m_cmd = header.m_protoHeader.m_cmd;
        m_protoHeader.m_seq = header.m_protoHeader.m_seq;
        m_paxosProtoHeader.m_cmd = header.m_paxosProtoHeader.m_cmd;
        m_paxosProtoHeader.m_length = header.m_paxosProtoHeader.m_length;
        m_paxosProtoHeader.m_resCode = header.m_paxosProtoHeader.m_resCode;
        return *this;
    }

    virtual ~PacketHeader(){}

    inline void clear(){
        m_protoHeader.m_length = PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE;
        m_protoHeader.m_cmd = MainProtoType_Paxos;
        m_protoHeader.m_seq = 0;
        m_paxosProtoHeader.m_cmd = 0;
        m_paxosProtoHeader.m_length = PAXOS_PROTO_HEADER_SIZE;
        m_paxosProtoHeader.m_resCode = RES_SUCCESS;
    }

    inline uint16_t getMainCmd() const 
    {
        return m_protoHeader.m_cmd;
    }

    inline void setMainCmd(uint16_t cmd)
    {
        m_protoHeader.m_cmd = cmd;
    }

    inline uint32_t getSeq() const
    {
        return m_protoHeader.m_seq;
    }

    inline void setSeq(uint32_t seq)
    {
        m_protoHeader.m_seq = seq;
    }

    inline uint16_t getLength() const
    {
        return m_protoHeader.m_length;
    }

    inline void setLength(uint16_t len)
    {
        m_protoHeader.m_length = len;
    }

    inline uint16_t getSubCmd() const 
    {
        return m_paxosProtoHeader.m_cmd;
    }

    inline void setSubCmd(uint16_t cmd) 
    {
        m_paxosProtoHeader.m_cmd = cmd;
    }

    inline uint16_t getSubLength() const
    {
        return m_paxosProtoHeader.m_length;
    }

    inline void setSubLength(uint16_t len)
    {
        m_paxosProtoHeader.m_length = len;
    }

    inline uint16_t getResCode() const 
    {
        return m_paxosProtoHeader.m_resCode;
    }

    inline void setResCode(uint16_t res)
    {
        m_paxosProtoHeader.m_resCode = res;
    }

    inline bool isSuccess() const
    {
        return m_paxosProtoHeader.m_resCode == RES_SUCCESS;
    }
};


//打包类
struct Encoder
{
public:
    Encoder():m_packetHeader(), pb(), headPack(pb, 0), bodyPack(pb, PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE){}
    
    Encoder(const Encoder &s):m_packetHeader(), pb(),headPack(pb, 0), bodyPack(pb, PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE)
    {
        m_packetHeader = s.m_packetHeader;
        headPack.replace(0, s.headPack.data(), PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE);
        bodyPack.push(s.bodyPack.data(), s.bodyPack.size());
    }

    virtual ~Encoder(){}
    
    const char* data()
    {
        return pb.data();
    }

    size_t size()
    {
        return PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE + bodySize();
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
        pb.resize(0 + PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE);
        m_packetHeader.clear();
    }

    PacketHeader& getHeader(){
        return m_packetHeader;
    }
	
    void setSeq(uint32_t seq){
        m_packetHeader.setSeq(seq);
    }

    void setResCode(uint16_t resCode = RES_SUCCESS){
        m_packetHeader.setResCode(resCode);
    }

    void serialize(uint16_t subCmd, const Marshallable &m)
    {
        m.marshal(bodyPack);
        m_packetHeader.setLength(size());
        m_packetHeader.setSubLength(size()-PROTO_HEADER_SIZE);
        m_packetHeader.setSubCmd(subCmd);
        headPack.replace_uint16(0, m_packetHeader.getLength());
        headPack.replace_uint16(2, m_packetHeader.getMainCmd());
        headPack.replace_uint32(4, m_packetHeader.getSeq());
        headPack.replace_uint16(8, m_packetHeader.getSubLength());
        headPack.replace_uint16(10, m_packetHeader.getSubCmd());
        headPack.replace_uint16(12, m_packetHeader.getResCode());
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
    static size_t minSize(){return PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE;}
    static size_t maxSize(){return MAX_PACKET_SIZE;}
    static size_t mainHeaderSize(){return PROTO_HEADER_SIZE;}
    static size_t headerSize(){return PROTO_HEADER_SIZE + PAXOS_PROTO_HEADER_SIZE;}
    static uint16_t pickLen(const void * data){uint16_t i = *((uint16_t*)data);return XHTONS(i);};
    static uint16_t pickCmd(const void * data){uint16_t i = *((uint16_t*)((char*)data+2));return XHTONS(i);};
    static uint32_t pickSeq(const char *data){uint32_t i = *((uint32_t*)((char*)data+4)); return XHTONL(i);};
    static uint16_t pickSubLen(const void * data){uint16_t i = *((uint16_t*)((char*)data+8));return XHTONS(i);};
    static uint16_t pickSubCmd(const void * data){uint16_t i = *((uint16_t*)((char*)data+10));return XHTONS(i);};
    static uint16_t pickResCode(const char *data){uint16_t i = *((uint16_t*)((char*)data+12)); return XHTONS(i);};

    Decoder(const char *data, uint16_t sz):up(data, sz){};
    virtual ~Decoder(){};

    void deserialize(PacketHeader& packetHeader, Marshallable& msg)
    {
        packetHeader.setLength(up.pop_uint16());
        packetHeader.setMainCmd(up.pop_uint16());
        packetHeader.setSeq(up.pop_uint32());
        packetHeader.setSubLength(up.pop_uint16());
        packetHeader.setSubCmd(up.pop_uint16());
        packetHeader.setResCode(up.pop_uint16());
        msg.unmarshal(up);
    }
private:
	Unpack up;
};

}