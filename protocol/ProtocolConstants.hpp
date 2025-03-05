#pragma once
#include <cstdint>

/* UUID string: cf270001-1d1c-4df3-8196-4f464c535643 */
constexpr uint8_t SENSOR_GATT_SERVICE_UUID[] = { 0x43, 0x56, 0x53, 0x4c, 0x46, 0x4f, 0x96, 0x81, 0xf3, 0x4d, 0x1c, 0x1d, 0x01, 0x00, 0x27, 0xcf };
constexpr uint8_t SENSOR_GATT_CHAR_RX_UUID[] = { 0x43, 0x56, 0x53, 0x4c, 0x46, 0x4f, 0x96, 0x81, 0xf3, 0x4d, 0x1c, 0x1d, 0x02, 0x00, 0x27, 0xcf };
constexpr uint8_t SENSOR_GATT_CHAR_TX_UUID[] = { 0x43, 0x56, 0x53, 0x4c, 0x46, 0x4f, 0x96, 0x81, 0xf3, 0x4d, 0x1c, 0x1d, 0x03, 0x00, 0x27, 0xcf };
constexpr uint16_t SENSOR_GATT_CHAR_RX_UUID16 = 0x0002;
constexpr uint16_t SENSOR_GATT_CHAR_TX_UUID16 = 0x0003;

constexpr uint8_t SENSOR_PROTOCOL_VERSION_MAJOR = 1;
constexpr uint8_t SENSOR_PROTOCOL_VERSION_MINOR = 1;

constexpr uint16_t SENSOR_MEAS_OFF = 0;
constexpr uint16_t SENSOR_MEAS_ON = 1;
constexpr uint16_t SENSOR_MEAS_SAMPLERATES_ECG[] = { SENSOR_MEAS_OFF, 125, 128, 200, 250, 256, 500, 512 };
constexpr uint16_t SENSOR_MEAS_SAMPLERATES_IMU[] = { SENSOR_MEAS_OFF, 13, 26, 52, 104, 208, 416, 833, 1666 };
constexpr uint16_t SENSOR_MEAS_TOGGLE[] = { SENSOR_MEAS_OFF, SENSOR_MEAS_ON };
constexpr uint16_t SENSOR_MEAS_PRESETS_ACTIVITY_INTERVALS[] = { SENSOR_MEAS_OFF, 1, 30, 60, 300, 900, 1800, 3600 };
