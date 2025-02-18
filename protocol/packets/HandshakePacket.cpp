#include "HandshakePacket.hpp"
#include "../ProtocolConstants.hpp"

HandshakePacket::HandshakePacket(uint8_t ref)
    : Packet(Packet::TypeHandshake, ref)
    , version_major(SENSOR_PROTOCOL_VERSION_MAJOR)
    , version_minor(SENSOR_PROTOCOL_VERSION_MINOR)
{
}

HandshakePacket::~HandshakePacket() 
{
}

bool HandshakePacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&version_major, sizeof(version_major));
    result &= stream.read(&version_minor, sizeof(version_minor));
    return result;
};

bool HandshakePacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&version_major, sizeof(version_major));
    result &= stream.write(&version_minor, sizeof(version_minor));
    return result;
}
