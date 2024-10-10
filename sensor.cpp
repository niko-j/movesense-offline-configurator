#include "sensor.h"
#include <QtLogging>

const QBluetoothUuid Sensor::serviceUuid = QUuid("0000b000-0000-1000-8000-00805f9b34fb");
const QBluetoothUuid Sensor::commandCharUuid = QUuid("0000b001-0000-1000-8000-00805f9b34fb");
const QBluetoothUuid Sensor::configCharUuid = QUuid("0000b002-0000-1000-8000-00805f9b34fb");
const QBluetoothUuid Sensor::dataCharUuid = QUuid("0000b003-0000-1000-8000-00805f9b34fb");

Sensor::Sensor(QObject* parent, const QBluetoothDeviceInfo& info)
    : QObject { parent }
    , _info(info)
    , _svc(nullptr)
    , _requestRef(1)
{
    _pController = QLowEnergyController::createCentral(info, this);
    connect(_pController, &QLowEnergyController::connected, this, &Sensor::onDeviceConnected);
    connect(_pController, &QLowEnergyController::disconnected, this, &Sensor::onDeviceDisconnected);
    connect(_pController, &QLowEnergyController::discoveryFinished, this, &Sensor::onFinishServiceDiscovery);
    connect(_pController, &QLowEnergyController::serviceDiscovered, this, &Sensor::onServiceDiscovered);
    connect(_pController, &QLowEnergyController::errorOccurred, this, &Sensor::onControllerError);
    _pController->setRemoteAddressType(QLowEnergyController::PublicAddress);
}

void Sensor::connectDevice()
{
    qInfo("Connecting to device %s", _info.name().toStdString().c_str());
    emit onStateChanged(State::Connecting);
    _pController->connectToDevice();
}

void Sensor::disconnectDevice()
{
    qInfo("Disconnecting from device %s", _info.name().toStdString().c_str());
    _pController->disconnectFromDevice();
}

bool Sensor::applyConfig(const SensorConfig& config)
{
    if(!_chars.count(configCharUuid))
        return false;

    const auto& c = _chars.value(configCharUuid);
    if(!c.isValid())
        return false;

    QByteArray data;
    config.write_to(data);

    _svc->writeCharacteristic(c, data);
    return true;
}

uint8_t Sensor::sendCommand(SensorCommands cmd)
{
    if(!_chars.count(commandCharUuid))
        return false;

    const auto& c = _chars.value(commandCharUuid);
    if(!c.isValid())
        return false;

    uint8_t cmdRef = _requestRef;

    _requestRef++;
    if(_requestRef == SENSOR_DATA_INVALID_REF)
        _requestRef++;

    QByteArray data = QByteArray();
    data.push_back(SensorCmdReadConfig);
    data.push_back(cmdRef);
    _svc->writeCharacteristic(c, data);

    return cmdRef;
}

void Sensor::onDeviceConnected()
{
    _pController->discoverServices();
}

void Sensor::onDeviceDisconnected()
{
    emit onStateChanged(State::Disconnected);
}

void Sensor::onServiceDiscovered(const QBluetoothUuid& uuid)
{
    qInfo("Found service: %s", uuid.toString().toStdString().c_str());

    if (uuid == serviceUuid)
    {
        qInfo("Offline mode GATT service found!");
        _svc = _pController->createServiceObject(uuid, this);
        connect(_svc, &QLowEnergyService::stateChanged, this, &Sensor::onServiceStateChanged);
        connect(_svc, &QLowEnergyService::characteristicChanged, this, &Sensor::onCharacteristicChanged);
        _svc->discoverDetails();
    }
}

void Sensor::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    switch(state)
    {
    case QLowEnergyService::RemoteServiceDiscovering:
    {
        emit onStateChanged(State::DiscoveringServices);
        break;
    }
    case QLowEnergyService::RemoteServiceDiscovered:
    {
        qInfo("Service discovered.");
        for(auto& c : _svc->characteristics())
        {
            qInfo("Found characteristic %s", c.uuid().toString().toStdString().c_str());
            _chars[c.uuid()] = c;

            if(c.uuid() == configCharUuid || c.uuid() == dataCharUuid)
            {
                auto desc = c.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
                if (desc.isValid())
                {
                    // Enable notifications
                    _svc->writeDescriptor(desc, QByteArray::fromHex("0100"));
                }
                else
                {
                    qInfo("Client characteristic configuration descriptor is not valid");
                }
            }
        }
        sendCommand(SensorCmdReadConfig);
        break;
    }
    default:
    {
        qInfo("Service state change: %d", state);
        break;
    }
    }
}

void Sensor::onCharacteristicChanged(const QLowEnergyCharacteristic& c, const QByteArray &value)
{
    qInfo("Characteristic changed: %s", c.uuid().toString().toStdString().c_str());

    SensorDataHeader header;
    if(!header.read_from_packet(value))
    {
        qInfo("Failed to read packet header");
        return;
    }

    qInfo("RECV packet (ref %u) (%lld bytes)", header.ref, value.size());

    if(c.uuid() == configCharUuid)
    {
        SensorConfig config;
        if(!config.read_from_packet(value))
        {
            emit onError(Error::ConfigReadFailure);
            return;
        }

        emit onConfigUpdated(config);
    }
    else if (c.uuid() == dataCharUuid)
    {
        static std::mutex bufferMutex;
        std::lock_guard lock(bufferMutex);

        if(!_buffers.contains(header.ref))
        {
            _buffers[header.ref] = DataTransmission {
                .received_bytes = 0,
                .bytes = QByteArray(header.total_len, 0)
            };
        }

        auto& buf = _buffers[header.ref];

        SensorDataBytes data;
        data.read_from_packet(value);

        if(header.offset + data.bytes.size() > buf.bytes.size()) {
            qWarning("Corrupted data packet");
            return;
        }

        memcpy(buf.bytes.data() + header.offset, data.bytes.data(), data.bytes.length());
        buf.received_bytes += data.bytes.size();

        if(buf.received_bytes == header.total_len)
        {
            emit onDataTransmissionCompleted(header.ref, data.bytes);
            _buffers.remove(header.ref);
        }
        else
        {
            emit onDataTransmissionProgressUpdate(header.ref, buf.received_bytes, header.total_len);
        }
    }
}

void Sensor::onControllerError(QLowEnergyController::Error error)
{
    qInfo("Controller error: %d", error);
    emit onError(ControllerError);
}

void Sensor::onFinishServiceDiscovery()
{
    qInfo("Ending service discovery");

    if (!_svc)
    {
        emit onError(UnsupportedDevice);
        disconnectDevice();
    }
    else
    {
        emit onStateChanged(State::Connected);
    }
}
