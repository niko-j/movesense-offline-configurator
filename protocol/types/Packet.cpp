#include "Packet.hpp"

Packet::Packet(Type packetType, uint8_t ref)
    : type(packetType)
    , reference(ref)
{
}

Packet::~Packet()
{
}

bool Packet::Read(ReadableBuffer& stream)
{
    bool result = true;
    result &= stream.read(&type, 1);
    result &= stream.read(&reference, 1);
    return result;
};

bool Packet::Write(WritableBuffer& stream)
{
    bool result = true;
    result &= stream.write(&type, 1);
    result &= stream.write(&reference, 1);
    return result;
}
