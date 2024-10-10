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

#define SENSOR_DATA_PAYLOAD_SIZE 128
#define SENSOR_DATA_INVALID_REF 0

enum SensorCommands : uint8_t
{
    SensorCmdReadConfig = 0x01,
    SensorCmdReportStatus = 0x02,
    SensorCmdGetSessions = 0x03,
    SensorCmdGetSessionLog = 0x04,
    SensorCmdClearSessionLogs = 0x05
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

struct SensorDataPart
{
    virtual void write_to(QByteArray& data) const = 0;
    virtual bool read_from_packet(const QByteArray& packet) = 0;
};

#define SENSOR_DATA_PART(len) \
    static constexpr size_t BYTE_SIZE = len; \
    void write_to(QByteArray& data) const; \
    bool read_from_packet(const QByteArray& packet);

struct SensorDataHeader : SensorDataPart
{
    SENSOR_DATA_PART(9)

    uint32_t offset = 0;
    uint32_t total_len = 0;
    uint8_t ref = SENSOR_DATA_INVALID_REF;
};

struct SensorConfig : SensorDataPart
{
    SENSOR_DATA_PART(15)

    uint8_t wakeup_behavior = 0;
    SensorSampleRates sample_rates = {};
    uint16_t sleep_delay = 0;
};

struct SensorStatus : SensorDataPart
{
    SENSOR_DATA_PART(9)

    uint32_t log_used = 0;
    uint32_t log_free = 0;
    uint8_t last_reset_reason = 0;
};

struct SensorDataBytes : SensorDataPart
{
    SENSOR_DATA_PART(SENSOR_DATA_PAYLOAD_SIZE);
    QByteArray bytes;
};

#endif // PROTOCOL_H
