#include "protocol.h"

void SensorDataHeader::write_to(QByteArray& data) const
{
    data.append((const char*) &offset, sizeof(offset));
    data.append((const char*) &total_len, sizeof(total_len));
    data.append((const char*) &ref, sizeof(ref));
}

bool SensorDataHeader::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= BYTE_SIZE)
    {
        const auto* data = packet.data();

        offset = *reinterpret_cast<const uint32_t*>(data);
        data += sizeof(offset);

        total_len = *reinterpret_cast<const uint32_t*>(data);
        data += sizeof(total_len);

        ref = *reinterpret_cast<const uint8_t*>(data);
    }
    return ref != SENSOR_DATA_INVALID_REF;
}

void SensorStatus::write_to(QByteArray& data) const
{
    data.append((const char*) &log_used, sizeof(log_used));
    data.append((const char*) &log_free, sizeof(log_free));
    data.append((const char*) &last_reset_reason, sizeof(last_reset_reason));
}

bool SensorStatus::read_from_packet(const QByteArray& packet)
{
    if(packet.size() == SensorDataHeader::BYTE_SIZE + BYTE_SIZE)
    {
        const auto* data = packet.data() + SensorDataHeader::BYTE_SIZE;
        log_used = *reinterpret_cast<const uint16_t*>(data);
        log_free = *reinterpret_cast<const uint16_t*>(data + 4);
        last_reset_reason = *reinterpret_cast<const uint16_t*>(data + 8);
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
    if(packet.size() == SensorDataHeader::BYTE_SIZE + BYTE_SIZE)
    {
        const auto* data = packet.data() + SensorDataHeader::BYTE_SIZE;
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

void SensorDataBytes::write_to(QByteArray& data) const
{
    data.append((const char*) bytes, qMin(sizeof(bytes), (size_t) data.length()));
}

bool SensorDataBytes::read_from_packet(const QByteArray& packet)
{
    if(packet.size() >= SensorDataHeader::BYTE_SIZE + 1)
    {
        size_t len = packet.size() - SensorDataHeader::BYTE_SIZE;
        const auto* data = packet.data() + SensorDataHeader::BYTE_SIZE;
        bytes.setRawData(data, len);
        return true;
    }
    return false;
}
