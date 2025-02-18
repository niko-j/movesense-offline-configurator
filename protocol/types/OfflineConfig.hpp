#pragma once
#include <cstdint>

struct OfflineConfig
{
    enum WakeUpBehavior : uint8_t
    {
        WakeUpAlwaysOn = 0U,
		WakeUpConnector = 1U,
		WakeUpMovement = 2U,
		WakeUpDoubleTap = 3U
    };
    
    enum Measurement
    {
        MeasECG = 0U,
		MeasHR = 1U,
		MeasRR = 2U,
		MeasAcc = 3U,
		MeasGyro = 4U,
		MeasMagn = 5U,
		MeasTemp = 6U,
		MeasActivity = 7U,
		MeasCount = 8U
    };

    enum OptionsFlags : uint8_t
    {
        OptionsLogTapGestures   = (1 << 0),
        OptionsLogShakeGestures = (1 << 1),
        OptionsCompressECG      = (1 << 2),
        OptionsShakeToConnect   = (1 << 3),
    };

    uint16_t sleepDelay = 0;
    uint8_t optionsFlags = 0;
    WakeUpBehavior wakeUpBehavior = WakeUpConnector;

    union {
        struct
        {
            uint16_t ECG;
            uint16_t HeartRate;
            uint16_t RtoR;            
            uint16_t Acc;
            uint16_t Gyro;
            uint16_t Magn;
            uint16_t Temp;
            uint16_t Activity;
        } bySensor;
        uint16_t array[MeasCount] = {};
    } measurementParams;
};
