#pragma once
#include "../types/Packet.hpp"

struct DataPacket : public Packet
{
    static constexpr size_t MAX_PAYLOAD = MAX_PACKET_SIZE - 10;

    uint32_t offset;
    uint32_t totalBytes;
    ReadableBuffer data;
    
    DataPacket(uint8_t ref);
    virtual ~DataPacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
