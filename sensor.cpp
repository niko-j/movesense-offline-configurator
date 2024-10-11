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
    if(_requestRef == SENSOR_INVALID_REF)
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

    SensorPacketHeader header;
    if(!header.read_from_packet(value))
    {
        qInfo("Failed to read packet header");
        return;
    }

    qInfo("RECV packet (ref %u) (%lld bytes)", header.requestReference, value.size());

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

        SensorPacketData dataInfo;
        if(!dataInfo.read_from_packet(value))
        {
            emit onError(Error::DataReadFailure);
            return;
        }

        if(!_buffers.contains(header.requestReference))
        {
            _buffers[header.requestReference] = DataTransmission {
                .received_bytes = 0,
                .bytes = QByteArray(dataInfo.totalBytes, 0)
            };
        }

        auto& buf = _buffers[header.requestReference];
        size_t payloadSize = value.size() - SENSOR_PACKET_DATA_OFFSET;

        if(dataInfo.offset + payloadSize > buf.bytes.size()) {
            qWarning("Corrupted data packet");
            return;
        }

        memcpy(buf.bytes.data() + dataInfo.offset, value.data() + SENSOR_PACKET_DATA_OFFSET, payloadSize);
        buf.received_bytes += payloadSize;

        if(buf.received_bytes == dataInfo.totalBytes)
        {
            emit onDataTransmissionCompleted(header.requestReference, buf.bytes);
            _buffers.remove(header.requestReference);
        }
        else
        {
            emit onDataTransmissionProgressUpdate(header.requestReference, buf.received_bytes, dataInfo.totalBytes);
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
