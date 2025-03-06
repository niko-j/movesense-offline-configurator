#include "DebugMessagePacket.hpp"

DebugMessagePacket::DebugMessagePacket(uint8_t ref)
    : Packet(Packet::TypeDebugMessage, ref)
    , level(4)
    , timestamp(0)
    , message(nullptr, 0)
{
}

DebugMessagePacket::~DebugMessagePacket()
{
}

bool DebugMessagePacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&level, sizeof(level));
    result &= stream.read(&timestamp, sizeof(timestamp));

    size_t len = stream.get_read_size() - stream.get_read_pos();
    message = ReadableBuffer(stream.get_read_ptr() + stream.get_read_pos(), len);

    return result;
};

bool DebugMessagePacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&level, sizeof(level));
    result &= stream.write(&timestamp, sizeof(timestamp));
    result &= message.write_to(stream);
    return result;
}
