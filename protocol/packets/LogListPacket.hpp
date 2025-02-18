#pragma once
#include "../types/Packet.hpp"

struct LogListPacket : public Packet
{
    static constexpr size_t MAX_ITEMS = 6;

    uint8_t count;
    bool complete;

    struct LogItem
    {
        uint32_t id;
        uint32_t size;
        uint64_t modified;
    };
    
    LogItem items[MAX_ITEMS];

    LogListPacket(uint8_t ref);
    virtual ~LogListPacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
