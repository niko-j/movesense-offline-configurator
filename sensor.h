#ifndef SENSOR_H
#define SENSOR_H

#include "protocol/Protocol.hpp"

#include <QObject>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyService>
#include <QLowEnergyController>

class Sensor : public QObject
{
    Q_OBJECT;

public:
    static const QBluetoothUuid serviceUuid;
    static const QBluetoothUuid rxUuid;
    static const QBluetoothUuid txUuid;

    explicit Sensor(QObject* parent, const QBluetoothDeviceInfo& info);

    void connectDevice();
    void disconnectDevice();

    uint8_t sendConfig(const OfflineConfig& conf);
    uint8_t sendCommand(CommandPacket::Command cmd, CommandPacket::CommandParams params);
    uint8_t sendPacket(Packet& packet);
    uint8_t syncTime();
    uint8_t handshake();

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
        UnsupportedVersion,
        ControllerError,
        ReadFailure,
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

    uint8_t nextRef() const;

signals:
    void onStateChanged(State state);
    void onConfigUpdated(const OfflineConfig& config);
    void onLogListReceived(uint8_t ref, const QList<LogListPacket::LogItem>& logs, bool complete);
    void onDataTransmissionCompleted(uint8_t cmdRef, const QByteArray& data);
    void onDataTransmissionProgressUpdate(uint8_t ref, uint32_t received_bytes, uint32_t total_bytes);
    void onStatusResponse(uint8_t ref, uint16_t status);
    void onError(Error err);

private:
    bool _timeSynced;
    uint8_t _handshake;
    QBluetoothDeviceInfo _info;
    QLowEnergyController* _pController;
    QLowEnergyService* _svc;
    QMap<QUuid, QLowEnergyCharacteristic> _chars;

    struct DataTransmission
    {
        size_t received_bytes = 0;
        QByteArray bytes;
    };
    QMap<uint8_t, DataTransmission> _buffers;
};

#endif // SENSOR_H
