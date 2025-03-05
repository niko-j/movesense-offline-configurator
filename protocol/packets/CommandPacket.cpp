#include "CommandPacket.hpp"

CommandPacket::CommandPacket(uint8_t ref, Command cmd, Params args)
    : Packet(Packet::TypeCommand, ref)
    , command(cmd)
    , params(args)
{
}

CommandPacket::~CommandPacket()
{
}

bool CommandPacket::Read(ReadableBuffer& stream)
{
    bool result = Packet::Read(stream);
    result &= stream.read(&command, sizeof(command));

    switch (command)
    {
    case CmdReadLog:
    {
        result &= stream.read(
            &params.readLog.logIndex,
            sizeof(params.readLog.logIndex));
        break;
    }
    case CmdStartDebugLogStream:
    {
        result &= stream.read(
            &params.debugLog.logLevel, 
            sizeof(params.debugLog.logLevel));
        result &= stream.read(
            &params.debugLog.sources,
            sizeof(params.debugLog.sources));
        break;
    }
    default: break;
    }

    return result;
};

bool CommandPacket::Write(WritableBuffer& stream)
{
    bool result = Packet::Write(stream);
    result &= stream.write(&command, sizeof(command));

    switch (command)
    {
    case CmdReadLog:
    {
        result &= stream.write(
            &params.readLog.logIndex,
            sizeof(params.readLog.logIndex));
        break;
    }
    case CmdStartDebugLogStream:
    {
        result &= stream.write(
            &params.debugLog.logLevel, 
            sizeof(params.debugLog.logLevel));
        result &= stream.write(
            &params.debugLog.sources,
            sizeof(params.debugLog.sources));
        break;
    }
    default: break;
    }

    return result;
}
