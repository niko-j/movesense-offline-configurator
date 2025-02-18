#pragma once
#include "../types/Packet.hpp"

struct TimePacket : public Packet
{
    int64_t time;

    TimePacket(uint8_t ref, int64_t t = 0);
    virtual ~TimePacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
