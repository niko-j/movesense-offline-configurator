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
        CmdDebugLastFault,
        CmdStartDebugLogStream,
        CmdStopDebugLogStream,
        CmdCount
    } command;

    union Params
    {
        struct ReadLogParams
        {
            uint16_t logIndex;
        } readLog;

        struct DebugLogParams
        {
            enum LogLevel : uint8_t
            {
                LogLevelFatal = 0,
                LogLevelError = 1,
                LogLevelWarning = 2,
                LogLevelInfo = 3,
                LogLevelVerbose = 4,
            } logLevel;

            enum LogSource : uint8_t
            {
                User = 0x01,
                System = 0x01,
            };
            uint8_t sources;

        } debugLog;
    } params;

    CommandPacket(uint8_t ref, Command cmd = CmdUnknown, Params args = {});
    virtual ~CommandPacket();
    virtual bool Read(ReadableBuffer& stream);
    virtual bool Write(WritableBuffer& stream);
};
