#include "sensor.h"
#include "protocol/packets/OfflineConfigPacket.hpp"
#include "protocol/packets/OfflineStatusPacket.hpp"
#include "protocol/packets/OfflineDataPacket.hpp"
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

uint8_t Sensor::sendConfig(const OfflineConfig& config)
{
    OfflineConfigPacket packet(nextRef());
    packet.config = config;
    return sendPacket(packet);
}

uint8_t Sensor::sendCommand(OfflineCommandPacket::Command cmd, const QByteArray& params)
{
    OfflineCommandPacket packet(nextRef(), cmd);
    packet.params.write(params.data(), params.size());
    return sendPacket(packet);
}


uint8_t Sensor::sendPacket(OfflinePacket& packet)
{
    if(!_chars.count(txUuid))
        return packet.INVALID_REF;

    const auto& c = _chars.value(txUuid);
    if(!c.isValid())
        return packet.INVALID_REF;

    QByteArray data(OfflinePacket::MAX_PACKET_SIZE, 0);
    WritableBuffer stream((uint8_t*) data.data(), data.size());
    packet.Write(stream);
    data.resize(stream.get_write_pos());
    _svc->writeCharacteristic(c, data);

    return packet.reference;
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
        sendCommand(OfflineCommandPacket::CmdReadConfig);
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
    OfflinePacket::Type type;
    uint8_t ref;
    ReadableBuffer buffer((const uint8_t*) value.data(), value.size());

    bool valid = buffer.read(&type, 1) && buffer.read(&ref, 1) && buffer.seek_read(0);
    if(!valid || ref == OfflinePacket::INVALID_REF)
    {
        qInfo("Received invalid packet");
        emit onError(Error::ReadFailure);
        return;
    }

    qInfo("RECV packet (ref %u) (type %u) (%lld bytes)", ref, type, value.size());

    switch(type)
    {
    case OfflinePacket::TypeStatus:
    {
        OfflineStatusPacket packet(ref, 0);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }
        qInfo("Received status %u for request %u", packet.status, ref);
        emit onStatusResponse(packet.reference, packet.status);
        break;
    }
    case OfflinePacket::TypeConfig:
    {
        OfflineConfigPacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }
        emit onConfigUpdated(packet.config);
        break;
    }
    case OfflinePacket::TypeLogList:
    {
        OfflineLogPacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        QList<OfflineLogPacket::LogItem> items;
        for(size_t i = 0; i < packet.count; i++)
            items.push_back(packet.items[i]);

        emit onLogListReceived(packet.reference, items, packet.complete);
        break;
    }
    case OfflinePacket::TypeData:
    {
        OfflineDataPacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        size_t len = buffer.get_read_size() - buffer.get_read_pos();
        QByteArray payload(len, 0);
        buffer.read(payload.data(), len);

        if(!_buffers.contains(ref))
        {
            _buffers[ref] = DataTransmission {
                .received_bytes = 0,
                .bytes = QByteArray(packet.totalBytes, 0)
            };
        }

        auto& buf = _buffers[ref];

        if(packet.offset + len > buf.bytes.size()) {
            qWarning("Corrupted data packet");
            return;
        }

        memcpy(buf.bytes.data() + packet.offset, payload.data(), len);
        buf.received_bytes += len;

        if(buf.received_bytes == packet.totalBytes)
        {
            emit onDataTransmissionCompleted(ref, buf.bytes);
            _buffers.remove(ref);
        }
        else
        {
            emit onDataTransmissionProgressUpdate(ref, buf.received_bytes, packet.totalBytes);
        }

        break;
    }
    default:
    {
        qInfo("Ignored packet %u of type %u", ref, type);
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
    if(ref == OfflinePacket::INVALID_REF)
        ref++;
    return ref;
}
