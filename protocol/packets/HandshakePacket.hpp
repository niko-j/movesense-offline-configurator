#pragma once
#include "../types/Packet.hpp"

struct HandshakePacket : public Packet
{
    uint8_t version_major;
    uint8_t version_minor;

    HandshakePacket(uint8_t ref);
    virtual ~HandshakePacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
