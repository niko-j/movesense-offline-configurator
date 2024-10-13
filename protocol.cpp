#include "protocol.h"

void SensorHeader::write_to(QByteArray& data) const
{
    data.append((const char*) &type, sizeof(type));
    data.append((const char*) &requestReference, sizeof(requestReference));
}

bool SensorHeader::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= BYTE_SIZE)
    {
        uint8_t t = packet.at(0);
        if(t > SensorPacketTypeUnknown && t < SensorPacketTypeCount)
            type = (SensorPacketType) t;
        else
            return false;

        requestReference = packet.at(1);
    }
    return requestReference != SENSOR_INVALID_REF;
}

void SensorCommand::write_to(QByteArray& data) const
{
    data.append((const char*) &command, sizeof(command));
    data.append(params);
}

bool SensorCommand::read_from_packet(const QByteArray& packet)
{
    if(packet.size() > SensorHeader::BYTE_SIZE && packet.size() <= SensorHeader::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeCommand)
            return false;

        const auto* data = packet.data() + SensorHeader::BYTE_SIZE;

        uint8_t cmd = *data;
        if(!(cmd > SensorCmdUnknown && cmd < SensorCmdCount))
            return false;
        command = (SensorCommands) cmd;
        data += 1;

        size_t params_len = packet.size() - SensorHeader::BYTE_SIZE - 1;
        if(params_len > 0)
            params.append(data, params_len);

        return true;
    }
    return false;
}

void SensorStatus::write_to(QByteArray& data) const
{
    data.append((const char*) &status, sizeof(status));
}

bool SensorStatus::read_from_packet(const QByteArray& packet)
{
    if(packet.size() == SensorHeader::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeStatus)
            return false;

        const auto* data = packet.data() + SensorHeader::BYTE_SIZE;
        status = *reinterpret_cast<const uint16_t*>(data);

        return true;
    }
    return false;
}

void SensorData::write_to(QByteArray& data) const
{
    data.append((const char*) &offset, sizeof(offset));
    data.append((const char*) &totalBytes, sizeof(totalBytes));
    data.append(bytes);
}

bool SensorData::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= SensorHeader::BYTE_SIZE + 8)
    {
        if(packet.at(0) != SensorPacketTypeData)
            return false;

        const auto* data = packet.data() + SensorHeader::BYTE_SIZE;
        offset = *reinterpret_cast<const uint32_t*>(data);
        totalBytes = *reinterpret_cast<const uint32_t*>(data + 4);
        data += 8;

        size_t data_len = packet.size() - SensorHeader::BYTE_SIZE - 8;
        if(data_len > 0)
            bytes.append(data, data_len);

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
    if(packet.size() == SensorHeader::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeConfig)
            return false;

        const auto* data = packet.data() + SensorHeader::BYTE_SIZE;
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

void SensorLogList::write_to(QByteArray& data) const
{
    // NOT IMPLEMENTED
}

bool SensorLogList::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= SensorHeader::BYTE_SIZE + 2 && packet.size() <= SensorHeader::BYTE_SIZE + BYTE_SIZE)
    {
        if(packet.at(0) != SensorPacketTypeLogList)
            return false;

        const auto* data = packet.data() + SensorHeader::BYTE_SIZE;

        size_t data_len = packet.size() - SensorHeader::BYTE_SIZE - 2;
        if(data_len != data[0] * sizeof(SensorLogItem))
            return false;

        count = data[0];
        complete = data[1];
        data += 2;

        for(size_t i = 0; i < count; i++)
        {
            SensorLogItem item = {
                .id = *reinterpret_cast<const uint32_t*>(data + sizeof(SensorLogItem) * i + 0),
                .size = *reinterpret_cast<const uint32_t*>(data + sizeof(SensorLogItem) * i + 4),
                .modified = *reinterpret_cast<const uint64_t*>(data + sizeof(SensorLogItem) * i + 8),
            };
            items.push_back(item);
        }
        return true;
    }
    return false;
}
