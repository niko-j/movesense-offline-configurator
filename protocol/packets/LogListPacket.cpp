#include "LogListPacket.hpp"

LogListPacket::LogListPacket(uint8_t ref)
    : Packet(Packet::TypeLogList, ref)
    , count(0)
    , complete(false)
{
}

LogListPacket::~LogListPacket()
{
}

bool LogListPacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&count, sizeof(count));
    result &= stream.read(&complete, sizeof(complete));

    for (uint8_t i = 0; i < count; i++)
    {
        result &= stream.read(&items[i].id, sizeof(items[i].id));
        result &= stream.read(&items[i].modified, sizeof(items[i].modified));
        result &= stream.read(&items[i].size, sizeof(items[i].size));
    }
    return result;
};

bool LogListPacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&count, sizeof(count));
    result &= stream.write(&complete, sizeof(complete));

    for (uint8_t i = 0; i < count; i++)
    {
        result &= stream.write(&items[i].id, sizeof(items[i].id));
        result &= stream.write(&items[i].modified, sizeof(items[i].modified));
        result &= stream.write(&items[i].size, sizeof(items[i].size));
    }
    return result;
}
