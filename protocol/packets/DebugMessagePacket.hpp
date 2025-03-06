#pragma once
#include "../types/Packet.hpp"

struct DebugMessagePacket : public Packet
{
    uint8_t level;
    uint32_t timestamp;
    ReadableBuffer message;

    static constexpr size_t MAX_MESSAGE_LEN = MAX_PACKET_SIZE - 5;
    
    DebugMessagePacket(uint8_t ref);
    virtual ~DebugMessagePacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
