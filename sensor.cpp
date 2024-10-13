#include "sensor.h"
#include <QtLogging>

const QBluetoothUuid Sensor::serviceUuid = QUuid("0000b001-0000-1000-8000-00805f9b34fb");
const QBluetoothUuid Sensor::rxUuid = QUuid("0000b002-0000-1000-8000-00805f9b34fb");
const QBluetoothUuid Sensor::txUuid = QUuid("0000b003-0000-1000-8000-00805f9b34fb");

Sensor::Sensor(QObject* parent, const QBluetoothDeviceInfo& info)
    : QObject { parent }
    , _info(info)
    , _svc(nullptr)
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

uint8_t Sensor::sendConfig(const SensorConfig& config)
{
    if(!_chars.count(txUuid))
        return false;

    const auto& c = _chars.value(txUuid);
    if(!c.isValid())
        return false;

    SensorHeader header;
    header.type = SensorPacketTypeConfig;
    header.requestReference = nextRef();

    QByteArray data;
    header.write_to(data);
    config.write_to(data);

    _svc->writeCharacteristic(c, data);
    return header.requestReference;
}

uint8_t Sensor::sendCommand(SensorCommands cmd, const QByteArray& params)
{
    if(!_chars.count(txUuid))
        return false;

    const auto& c = _chars.value(txUuid);
    if(!c.isValid())
        return false;

    SensorHeader header;
    header.type = SensorPacketTypeCommand;
    header.requestReference = nextRef();

    SensorCommand command;
    command.command = cmd;
    command.params = params;

    QByteArray data;
    header.write_to(data);
    command.write_to(data);

    _svc->writeCharacteristic(c, data);
    return header.requestReference;
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

            if(c.uuid() == rxUuid)
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

    SensorHeader header;
    if(!header.read_from_packet(value))
    {
        qInfo("Failed to read packet header");
        return;
    }

    qInfo("RECV packet (ref %u) (type %u) (%lld bytes)",
          header.requestReference,
          header.type,
          value.size());

    switch(header.type)
    {
    case SensorPacketTypeStatus:
    {
        SensorStatus status;
        if(!status.read_from_packet(value))
        {
            emit onError(Error::ReadFailure);
            return;
        }
        qInfo("Received status %u for request %u",
              status.status, header.requestReference);
        emit onStatusResponse(header.requestReference, status.status);
        break;
    }
    case SensorPacketTypeConfig:
    {
        SensorConfig config;
        if(!config.read_from_packet(value))
        {
            emit onError(Error::ReadFailure);
            return;
        }
        emit onConfigUpdated(config);
        break;
    }
    case SensorPacketTypeLogList:
    {
        SensorLogList list;
        if(!list.read_from_packet(value))
        {
            emit onError(Error::ReadFailure);
            return;
        }
        emit onLogListReceived(header.requestReference, list.items, list.complete);
        break;
    }
    case SensorPacketTypeData:
    {
        SensorData data;
        if(!data.read_from_packet(value))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        if(!_buffers.contains(header.requestReference))
        {
            _buffers[header.requestReference] = DataTransmission {
                .received_bytes = 0,
                .bytes = QByteArray(data.totalBytes, 0)
            };
        }

        auto& buf = _buffers[header.requestReference];
        size_t payloadSize = data.bytes.size();

        if(data.offset + payloadSize > buf.bytes.size()) {
            qWarning("Corrupted data packet");
            return;
        }

        memcpy(buf.bytes.data() + data.offset, value.data() + SENSOR_PACKET_DATA_OFFSET, payloadSize);
        buf.received_bytes += payloadSize;

        if(buf.received_bytes == data.totalBytes)
        {
            emit onDataTransmissionCompleted(header.requestReference, buf.bytes);
            _buffers.remove(header.requestReference);
        }
        else
        {
            emit onDataTransmissionProgressUpdate(header.requestReference, buf.received_bytes, data.totalBytes);
        }

        break;
    }
    default:
    {
        qInfo("Ignored packet %u", header.requestReference);
        return;
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

uint8_t Sensor::nextRef() const
{
    static uint8_t ref = 0;
    ref++;
    if(ref == SENSOR_INVALID_REF)
        ref++;
    return ref;
}
