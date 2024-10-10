#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <mutex>

class Scanner : public QObject
{
    Q_OBJECT
public:
    explicit Scanner(QObject *parent = nullptr);

    void start();
    void stop();

    QList<QBluetoothDeviceInfo> listDevices();

    enum State
    {
        Stopped,
        Scanning
    };

signals:
    void deviceListUpdated(const QList<QBluetoothDeviceInfo>& devices);
    void stateChanged(State state);

private:

    void onDeviceFound(const QBluetoothDeviceInfo& info);
    void onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error err);
    void onDiscoveryStopped();

    QBluetoothDeviceDiscoveryAgent* _agent;
    QList<QBluetoothDeviceInfo> _devices;
    std::mutex _mtx;
};

#endif // SCANNER_H
