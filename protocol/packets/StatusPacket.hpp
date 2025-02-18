#pragma once
#include "../types/Packet.hpp"

struct StatusPacket : public Packet
{
    uint16_t status;

    StatusPacket(uint8_t ref, uint16_t statusCode = 0);
    virtual ~StatusPacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
