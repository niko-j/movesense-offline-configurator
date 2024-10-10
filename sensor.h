#ifndef SENSOR_H
#define SENSOR_H

#include "protocol.h"
#include <QObject>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyService>
#include <QLowEnergyController>

class Sensor : public QObject
{
    Q_OBJECT;

public:
    static const QBluetoothUuid serviceUuid;
    static const QBluetoothUuid commandCharUuid;
    static const QBluetoothUuid configCharUuid;
    static const QBluetoothUuid dataCharUuid;

    explicit Sensor(QObject* parent, const QBluetoothDeviceInfo& info);

    void connectDevice();
    void disconnectDevice();

    bool applyConfig(const SensorConfig& conf);
    uint8_t sendCommand(SensorCommands cmd);

    std::vector<uint8_t> downloadData();

    enum State
    {
        Disconnected,
        Connecting,
        DiscoveringServices,
        Connected,
    };

    enum Error
    {
        UnsupportedDevice,
        ControllerError,
        ConfigReadFailure,
        DataReadFailure,
    };

private:
    void discoverServices();
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onServiceDiscovered(const QBluetoothUuid& uuid);
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray &value);
    void onControllerError(QLowEnergyController::Error error);
    void onFinishServiceDiscovery();

signals:
    void onStateChanged(State state);
    void onConfigUpdated(const SensorConfig& config);
    void onDataTransmissionCompleted(uint8_t cmdRef, const QByteArray& data);
    void onDataTransmissionProgressUpdate(uint8_t ref, uint32_t received_bytes, uint32_t total_bytes);
    void onError(Error err);

private:
    QBluetoothDeviceInfo _info;
    QLowEnergyController* _pController;
    QLowEnergyService* _svc;

    uint8_t _requestRef;
    QMap<QUuid, QLowEnergyCharacteristic> _chars;

    struct DataTransmission
    {
        size_t received_bytes = 0;
        QByteArray bytes;
    };

    QMap<uint8_t, DataTransmission> _buffers;
};

#endif // SENSOR_H
