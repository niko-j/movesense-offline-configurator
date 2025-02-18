#include "TimePacket.hpp"

TimePacket::TimePacket(uint8_t ref, int64_t t)
    : Packet(Packet::TypeTime, ref)
    , time(t)
{
}

TimePacket::~TimePacket()
{
}

bool TimePacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&time, sizeof(time));
    return result;
};

bool TimePacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&time, sizeof(time));
    return result;
}
