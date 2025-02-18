#pragma once
#include "../types/Packet.hpp"

struct CommandPacket : public Packet
{
    enum Command : uint8_t
    {
        CmdUnknown,
        CmdReadConfig,
        CmdListLogs,
        CmdReadLog,
        CmdClearLogs,
        CmdCount
    } command;

    union CommandParams
    {
        struct 
        {
            uint16_t logIndex;
        } ReadLogParams;
    } params;

    CommandPacket(uint8_t ref, Command cmd = CmdUnknown, CommandParams args = {});
    virtual ~CommandPacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
