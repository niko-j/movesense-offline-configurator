#include "CommandPacket.hpp"

CommandPacket::CommandPacket(uint8_t ref, Command cmd, CommandParams args)
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
            &params.ReadLogParams.logIndex,
            sizeof(params.ReadLogParams.logIndex));
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
            &params.ReadLogParams.logIndex,
            sizeof(params.ReadLogParams.logIndex));
        break;
    }
    default: break;
    }

    return result;
}
