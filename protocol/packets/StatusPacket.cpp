#include "StatusPacket.hpp"

StatusPacket::StatusPacket(uint8_t ref, uint16_t statusCode)
    : Packet(Packet::TypeStatus, ref)
    , status(statusCode)
{
}

StatusPacket::~StatusPacket() 
{
}

bool StatusPacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&status, sizeof(status));
    return result;
};

bool StatusPacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&status, sizeof(status));
    return result;
}
