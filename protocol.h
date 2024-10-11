#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <QByteArray>
#include <QList>

#define SENSOR_OFF 0
#define SENSOR_ON 1

#define SENSOR_SAMPLERATES_ECG { SENSOR_OFF, 125, 128, 200, 250, 256, 500, 512 }
#define SENSOR_SAMPLERATES_IMU { SENSOR_OFF, 13, 26, 52, 104, 208, 416, 833, 1666 }
#define SENSOR_SAMPLERATES_ONOFF { SENSOR_OFF, SENSOR_ON }

#define SENSOR_PAYLOAD_SIZE 120
#define SENSOR_INVALID_REF 0

struct SensorDataSection
{
    virtual void write_to(QByteArray& data) const = 0;
    virtual bool read_from_packet(const QByteArray& packet) = 0;
};

#define SENSOR_DATA_SECTION_LENGTH(len) \
static constexpr size_t BYTE_SIZE = len; \
    void write_to(QByteArray& data) const; \
    bool read_from_packet(const QByteArray& packet);

enum SensorCommands : uint8_t
{
    SensorCmdReadConfig = 0x01,
    SensorCmdReportStatus = 0x02,
    SensorCmdGetSessions = 0x03,
    SensorCmdGetSessionLog = 0x04,
    SensorCmdClearSessionLogs = 0x05
};

enum SensorPacketType : uint8_t
{
    SensorPacketTypeUnknown = 0,
    SensorPacketTypeData,
    SensorPacketTypeStatus,
    SensorPacketTypeCount,
};

enum SensorMeasurements
{
    SensorMeasECG,
    SensorMeasHR,
    SensorMeasAccel,
    SensorMeasGyro,
    SensorMeasMagn,
    SensorMeasTemp,

    SensorMeasCount
};

enum SensorWakeUp
{
    SensorWakeUpAlwaysOn,
    SensorWakeUpHR,
    SensorWakeUpMovement,
    SensorWakeUpSingleTapOn,
    SensorWakeUpSingleTapOnOff,
    SensorWakeUpDoubleTapOn,
    SensorWakeUpDoubleTapOnOff,
};

union SensorSampleRates
{
    struct {
        uint16_t
            ECG,
            HeartRate,
            Acceleration,
            Gyro,
            Magnetometer,
            Temperature;
    } by_sensor;
    uint16_t data[SensorMeasCount];
};

struct SensorPacketHeader : SensorDataSection
{
    SENSOR_DATA_SECTION_LENGTH(2);
    SensorPacketType type = SensorPacketTypeUnknown;
    uint8_t requestReference = SENSOR_INVALID_REF;
};

struct SensorPacketStatus : SensorDataSection
{
    SENSOR_DATA_SECTION_LENGTH(2);
    uint16_t status;
};

struct SensorPacketData : SensorDataSection
{
    SENSOR_DATA_SECTION_LENGTH(8);
    uint32_t offset;
    uint32_t totalBytes;
};

struct SensorConfig : SensorDataSection
{
    SENSOR_DATA_SECTION_LENGTH(15);
    uint8_t wakeup_behavior = 0;
    SensorSampleRates sample_rates = {};
    uint16_t sleep_delay = 0;
};

constexpr size_t SENSOR_PACKET_DATA_OFFSET = (SensorPacketHeader::BYTE_SIZE + SensorPacketData::BYTE_SIZE);

#endif // PROTOCOL_H
