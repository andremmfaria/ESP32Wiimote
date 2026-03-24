#ifndef ESP32_WIIMOTE_COMMAND_DISPATCH_REASON_H
#define ESP32_WIIMOTE_COMMAND_DISPATCH_REASON_H

#include <stdint.h>

// Shared command dispatch outcomes used across transport layers (Serial and Web API).
enum class CommandDispatchReason : uint8_t {
    Ok,
    Queued,

    NotConnected,
    Locked,
    UnknownCommand,
    BadArgument,
    BadCredentials,
    MissingArgument,
    Rejected,
    PolicyBlocked,
    ControllerNotInitialized,
    DiscoveryNotActive,
    DiscoveryAlreadyActive,
    NoActiveConnection,
    InvalidVerb,
    WifiControlDisabled,

    Unauthorized,
    NotFound,
    QueueFull,
    InternalError,
};

const char *commandDispatchReasonToSerialCode(CommandDispatchReason reason);
const char *commandDispatchReasonToWebMessage(CommandDispatchReason reason);
const char *commandDispatchReasonToOpenApiDescription(CommandDispatchReason reason);
int commandDispatchReasonToHttpStatus(CommandDispatchReason reason);
bool commandDispatchReasonIsSuccess(CommandDispatchReason reason);

#endif  // ESP32_WIIMOTE_COMMAND_DISPATCH_REASON_H