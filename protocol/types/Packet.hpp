#pragma once
#include "../utils/Buffers.hpp"

#ifndef OFFLINE_BLE_MTU
#define OFFLINE_BLE_MTU 161
#endif

struct Packet
{
    static constexpr uint8_t INVALID_REF = 0;
    static constexpr uint32_t MAX_PACKET_SIZE = OFFLINE_BLE_MTU - 3;

    enum Type : uint8_t
    {
        TypeHandshake = 0x00,
        TypeCommand = 0x01,
        TypeStatus = 0x02,
        TypeData = 0x03,
        TypeOfflineConfig = 0x04,
        TypeLogList = 0x05,
        TypeTime = 0x06,
        TypeDebugMessage = 0x07,
    } type;
    uint8_t reference;

    Packet(Type packetType, uint8_t ref);
    virtual ~Packet();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
