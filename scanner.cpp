#include "scanner.h"
#include <QMessageBox>

Scanner::Scanner(QObject *parent)
    : QObject { parent }
{
    _agent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(_agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &Scanner::onDeviceFound);
    connect(_agent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &Scanner::onDiscoveryError);
    connect(_agent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &Scanner::onDiscoveryStopped);
    connect(_agent, &QBluetoothDeviceDiscoveryAgent::finished, this, &Scanner::onDiscoveryStopped);
}

void Scanner::start()
{
    if(!_agent->isActive())
    {
        _devices.clear();
        emit deviceListUpdated(_devices);

        _agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
        emit stateChanged(State::Scanning);
    }
}

void Scanner::stop()
{
    if(_agent->isActive())
    {
        _agent->stop();
    }
}

QList<QBluetoothDeviceInfo> Scanner::listDevices()
{
    std::lock_guard lck(_mtx);
    return _devices;
}

void Scanner::onDeviceFound(const QBluetoothDeviceInfo& info)
{
    if (!info.name().contains("Movesense"))
        return;

    std::lock_guard lck(_mtx);

    bool exists = false;
    for(auto& dev : _devices)
    {
        if (dev.deviceUuid() == info.deviceUuid())
        {
            dev = info;
            exists = true;
            break;
        }
    }

    if(!exists)
        _devices.push_back(info);

    emit deviceListUpdated(_devices);
}

void Scanner::onDiscoveryError(QBluetoothDeviceDiscoveryAgent::Error err)
{
    QString msg = QString::asprintf("Device discovery agent reported an error: %u", err);
    QMessageBox::warning(nullptr, "Warning", msg, QMessageBox::Ok);
}

void Scanner::onDiscoveryStopped()
{
    emit stateChanged(State::Stopped);
}
