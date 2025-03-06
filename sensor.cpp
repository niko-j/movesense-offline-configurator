#include "sensor.h"
#include <QtLogging>

const QBluetoothUuid Sensor::serviceUuid = QUuid::fromBytes(SENSOR_GATT_SERVICE_UUID, QSysInfo::LittleEndian);

// Swapped TX <-> RX for clients
const QBluetoothUuid Sensor::txUuid = QUuid::fromBytes(SENSOR_GATT_CHAR_RX_UUID, QSysInfo::LittleEndian);
const QBluetoothUuid Sensor::rxUuid = QUuid::fromBytes(SENSOR_GATT_CHAR_TX_UUID, QSysInfo::LittleEndian);

constexpr uint8_t DEBUG_LOG_STREAM_REF = 10;

Sensor::Sensor(QObject* parent, const QBluetoothDeviceInfo& info)
    : QObject { parent }
    , _timeSynced(false)
    , _handshake(Packet::INVALID_REF)
    , _debugRequest(Packet::INVALID_REF)
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

uint8_t Sensor::sendCommand(CommandPacket::Command cmd, CommandPacket::Params params)
{
    CommandPacket packet(nextRef(), cmd, params);
    return sendPacket(packet);
}

uint8_t Sensor::sendPacket(Packet& packet)
{
    if(!_chars.count(txUuid))
        return packet.INVALID_REF;

    const auto& c = _chars.value(txUuid);
    if(!c.isValid())
        return packet.INVALID_REF;

    QByteArray data(Packet::MAX_PACKET_SIZE, 0);
    WritableBuffer stream((uint8_t*) data.data(), data.size());
    packet.Write(stream);
    data.resize(stream.get_write_pos());
    _svc->writeCharacteristic(c, data);

    return packet.reference;
}

uint8_t Sensor::syncTime()
{
    uint64_t timestamp_in_microseconds = time(0) * 1000000UL;
    TimePacket packet(nextRef(), timestamp_in_microseconds);
    return sendPacket(packet);
}

uint8_t Sensor::handshake()
{
    HandshakePacket packet(nextRef());
    return sendPacket(packet);
}

void Sensor::startStreamingLogMessages()
{
    CommandPacket::Params params = {
        .debugLog = {
            .logLevel = CommandPacket::Params::DebugLogParams::LogLevelInfo,
            .sources = CommandPacket::Params::DebugLogParams::System |
                       CommandPacket::Params::DebugLogParams::User
        }
    };

    // Use fixed packet reference to avoid conflicts with other packets
    CommandPacket packet(DEBUG_LOG_STREAM_REF, CommandPacket::CmdStartDebugLogStream, params);
    sendPacket(packet);
}

void Sensor::stopStreamingLogMessages()
{
    sendCommand(CommandPacket::CmdStopDebugLogStream, {});
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
        _handshake = handshake();
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
    Packet::Type type;
    uint8_t ref;
    ReadableBuffer buffer((const uint8_t*) value.data(), value.size());

    bool valid = buffer.read(&type, 1) && buffer.read(&ref, 1) && buffer.seek_read(0);
    if(!valid || ref == Packet::INVALID_REF)
    {
        qInfo("Received invalid packet");
        emit onError(Error::ReadFailure);
        return;
    }

    qInfo("RECV packet (ref %u) (type %u) (%lld bytes)", ref, type, value.size());

    switch(type)
    {
    case Packet::TypeHandshake:
    {
        HandshakePacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        qInfo("Handshake - Protocol version %u.%u", packet.version_major, packet.version_minor);

        if(packet.version_major == 1 && packet.version_minor >= 1)
            _debugRequest = sendCommand(CommandPacket::CmdDebugLastFault, {});
        else
            sendCommand(CommandPacket::CmdReadConfig, {});

        break;
    }
    case Packet::TypeStatus:
    {
        StatusPacket packet(ref, 0);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        qInfo("Received status %u for request %u", packet.status, ref);
        emit onStatusResponse(packet.reference, packet.status);
        break;
    }
    case Packet::TypeOfflineConfig:
    {
        OfflineConfigPacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        if(!_timeSynced)
        {
            _timeSynced = true;
            syncTime();
        }

        emit onConfigUpdated(packet.config);
        break;
    }
    case Packet::TypeLogList:
    {
        LogListPacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        QList<LogListPacket::LogItem> items;
        for(size_t i = 0; i < packet.count; i++)
            items.push_back(packet.items[i]);

        emit onLogListReceived(packet.reference, items, packet.complete);
        break;
    }
    case Packet::TypeData:
    {   
        DataPacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }

        size_t len = packet.data.get_read_size();
        auto data = packet.data.get_read_ptr();
        QByteArray payload((const char*) data, len);

        if(ref == _debugRequest)
        {
            if(payload.size() >= sizeof(uint64_t))
            {
                uint64_t lastReset = *reinterpret_cast<const uint64_t*>(payload.data());

                if(lastReset > 0)
                {
                    qInfo("Debug info:");
                    payload.erase(payload.constBegin(), payload.constBegin() + sizeof(lastReset));
                    for(size_t i = 0; i < payload.size(); i++)
                    {
                        if(payload[i] != '\0')
                        {
                            size_t len = strnlen(payload.data() + i, payload.size() - i);
                            std::string line = std::string(payload.data() + i, len);
                            if(!line.empty())
                                qInfo("\t%s", line.c_str());
                            i += len;
                        }
                    }
                }
            }
            sendCommand(CommandPacket::CmdReadConfig, {});
            return;
        }

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
    case Packet::TypeDebugMessage:
    {
        DebugMessagePacket packet(ref);
        if(!packet.Read(buffer))
        {
            emit onError(Error::ReadFailure);
            return;
        }
        emit onReceiveLogStream(packet);
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
    const uint8_t begin = 100;
    const uint8_t end = 200;
    static uint8_t ref = begin;
    if(ref + 1 == end)
        ref = begin;
    else
        ref += 1;
    return ref;
}
