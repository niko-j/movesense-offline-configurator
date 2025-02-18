#pragma once
#include "../types/Packet.hpp"
#include "../types/OfflineConfig.hpp"

struct OfflineConfigPacket : public Packet
{
    OfflineConfig config;

    OfflineConfigPacket(uint8_t ref, OfflineConfig conf = {});
    virtual ~OfflineConfigPacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
