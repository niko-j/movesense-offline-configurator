#include "protocol.h"

void SensorPacketHeader::write_to(QByteArray& data) const
{
    data.append((const char*) &type, sizeof(type));
    data.append((const char*) &requestReference, sizeof(requestReference));
}

bool SensorPacketHeader::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= BYTE_SIZE)
    {
        uint8_t t = packet.at(0);
        if(t > SensorPacketTypeCount && t < SensorPacketTypeUnknown)
            type = (SensorPacketType) t;
        else
            return false;

        requestReference = packet.at(1);
    }
    return requestReference != SENSOR_INVALID_REF;
}

void SensorPacketStatus::write_to(QByteArray& data) const
{
    data.append((const char*) &status, sizeof(status));
}

bool SensorPacketStatus::read_from_packet(const QByteArray& packet)
{
    if(packet.size() == SensorPacketHeader::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeStatus)
            return false;

        const auto* data = packet.data() + SensorPacketHeader::BYTE_SIZE;
        status = *reinterpret_cast<const uint16_t*>(data);

        return true;
    }
    return false;
}

void SensorPacketData::write_to(QByteArray& data) const
{
    data.append((const char*) &offset, sizeof(offset));
    data.append((const char*) &totalBytes, sizeof(totalBytes));
}

bool SensorPacketData::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= SensorPacketHeader::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeData)
            return false;

        const auto* data = packet.data() + SensorPacketHeader::BYTE_SIZE;
        offset = *reinterpret_cast<const uint32_t*>(data);
        totalBytes = *reinterpret_cast<const uint32_t*>(data + 4);

        return true;
    }
    return false;
}

void SensorConfig::write_to(QByteArray& data) const
{
    data.append((const char*) &wakeup_behavior, sizeof(wakeup_behavior));
    data.append((const char*) sample_rates.data, sizeof(sample_rates.data));
    data.append((const char*) &sleep_delay, sizeof(sleep_delay));
}

bool SensorConfig::read_from_packet(const QByteArray& packet)
{
    if(packet.size() == SensorPacketHeader::BYTE_SIZE + SensorPacketData::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeData)
            return false;

        const auto* data = packet.data() + SensorPacketHeader::BYTE_SIZE + SensorPacketData::BYTE_SIZE;
        wakeup_behavior = data[0]; data += 1;
        for(auto i = 0; i < SensorMeasCount; i++)
        {
            sample_rates.data[i] = *reinterpret_cast<const uint16_t*>(data);
            data += sizeof(uint16_t);
        }
        sleep_delay = *reinterpret_cast<const uint16_t*>(data);
        return true;
    }
    return false;
}

