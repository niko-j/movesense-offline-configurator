#include "DataPacket.hpp"

DataPacket::DataPacket(uint8_t ref)
    : Packet(Packet::TypeData, ref)
    , offset(0)
    , totalBytes(0)
    , data(nullptr, 0)
{
}

DataPacket::~DataPacket()
{
}

bool DataPacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&offset, sizeof(offset));
    result &= stream.read(&totalBytes, sizeof(totalBytes));

    size_t len = stream.get_read_size() - stream.get_read_pos();
    data = ReadableBuffer(stream.get_read_ptr() + stream.get_read_pos(), len);

    return result;
};

bool DataPacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&offset, sizeof(offset));
    result &= stream.write(&totalBytes, sizeof(totalBytes));
    result &= data.write_to(stream);
    return result;
}
