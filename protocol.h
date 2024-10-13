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

struct SensorPacketSection
{
    virtual void write_to(QByteArray& data) const = 0;
    virtual bool read_from_packet(const QByteArray& packet) = 0;
};

#define SENSOR_PACKET_SECTION(Length) \
static constexpr size_t BYTE_SIZE = Length; \
    void write_to(QByteArray& data) const; \
    bool read_from_packet(const QByteArray& packet);

enum SensorCommands : uint8_t
{
    SensorCmdUnknown = 0,

    SensorCmdReadConfig,
    SensorCmdListLogs,
    SensorCmdListLogById,
    SensorCmdClearLogs,

    SensorCmdCount
};

enum SensorPacketType : uint8_t
{
    SensorPacketTypeUnknown = 0,

    SensorPacketTypeCommand,
    SensorPacketTypeStatus,
    SensorPacketTypeData,
    SensorPacketTypeConfig,
    SensorPacketTypeLogList,

    SensorPacketTypeCount
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
    SensorWakeUpConnector,
    SensorWakeUpMovement,
    SensorWakeUpSingleTapOn,
    SensorWakeUpDoubleTapOn,
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

struct SensorHeader : SensorPacketSection
{
    SENSOR_PACKET_SECTION(2);
    SensorPacketType type = SensorPacketTypeUnknown;
    uint8_t requestReference = SENSOR_INVALID_REF;
};

struct SensorCommand : SensorPacketSection
{
    SENSOR_PACKET_SECTION(1 + 32);
    SensorCommands command;
    QByteArray params;
};

struct SensorStatus : SensorPacketSection
{
    SENSOR_PACKET_SECTION(2);
    uint16_t status;
};

struct SensorData : SensorPacketSection
{
    SENSOR_PACKET_SECTION(8 + SENSOR_PAYLOAD_SIZE);
    uint32_t offset;
    uint32_t totalBytes;
    QByteArray bytes;
};

struct SensorConfig : SensorPacketSection
{
    SENSOR_PACKET_SECTION(15);
    uint8_t wakeup_behavior = 0;
    SensorSampleRates sample_rates = {};
    uint16_t sleep_delay = 0;
};

struct SensorLogItem
{
    uint32_t id;
    uint32_t size;
    uint64_t modified;
};

struct SensorLogList : SensorPacketSection
{
    SENSOR_PACKET_SECTION(2 + 96);
    uint8_t count;
    bool complete;
    QList<SensorLogItem> items;
};


constexpr size_t SENSOR_PACKET_DATA_OFFSET = (SensorHeader::BYTE_SIZE + 8);

#endif // PROTOCOL_H
