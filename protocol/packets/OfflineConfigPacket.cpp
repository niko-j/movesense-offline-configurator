#include "OfflineConfigPacket.hpp"

OfflineConfigPacket::OfflineConfigPacket(uint8_t ref, OfflineConfig conf)
    : Packet(Packet::TypeOfflineConfig, ref)
    , config(conf)
{
}
OfflineConfigPacket::~OfflineConfigPacket()
{
}

bool OfflineConfigPacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&config.wakeUpBehavior, 1);
    result &= stream.read(&config.sleepDelay, 2);
    result &= stream.read(&config.optionsFlags, 1);
    result &= stream.read(&config.measurementParams, OfflineConfig::MeasCount * 2);
    return result;
};

bool OfflineConfigPacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&config.wakeUpBehavior, 1);
    result &= stream.write(&config.sleepDelay, 2);
    result &= stream.write(&config.optionsFlags, 1);
    result &= stream.write(&config.measurementParams, OfflineConfig::MeasCount * 2);
    return result;
}
