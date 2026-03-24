#include "command_dispatch_reason.h"

const char *commandDispatchReasonToSerialCode(CommandDispatchReason reason) {
    switch (reason) {
        case CommandDispatchReason::NotConnected:
            return "not_connected";
        case CommandDispatchReason::Locked:
            return "locked";
        case CommandDispatchReason::UnknownCommand:
            return "unknown_command";
        case CommandDispatchReason::BadArgument:
            return "bad_argument";
        case CommandDispatchReason::BadCredentials:
            return "bad_credentials";
        case CommandDispatchReason::MissingArgument:
            return "missing_argument";
        case CommandDispatchReason::Rejected:
            return "rejected";
        case CommandDispatchReason::PolicyBlocked:
            return "policy_blocked";
        case CommandDispatchReason::ControllerNotInitialized:
            return "controller_not_initialized";
        case CommandDispatchReason::DiscoveryNotActive:
            return "discovery_not_active";
        case CommandDispatchReason::DiscoveryAlreadyActive:
            return "discovery_already_active";
        case CommandDispatchReason::NoActiveConnection:
            return "no_active_connection";
        case CommandDispatchReason::InvalidVerb:
            return "invalid_verb";
        case CommandDispatchReason::WifiControlDisabled:
            return "wifi_control_disabled";
        case CommandDispatchReason::Unauthorized:
            return "unauthorized";
        case CommandDispatchReason::NotFound:
            return "not_found";
        case CommandDispatchReason::QueueFull:
            return "queue_full";
        case CommandDispatchReason::Ok:
        case CommandDispatchReason::Queued:
        default:
            return "internal";
    }
}

const char *commandDispatchReasonToWebMessage(CommandDispatchReason reason) {
    switch (reason) {
        case CommandDispatchReason::Ok:
            return "ok";
        case CommandDispatchReason::Queued:
            return "accepted";
        case CommandDispatchReason::NotConnected:
            return "wiimote not connected";
        case CommandDispatchReason::Locked:
            return "locked";
        case CommandDispatchReason::UnknownCommand:
            return "unknown command";
        case CommandDispatchReason::BadArgument:
            return "invalid argument";
        case CommandDispatchReason::BadCredentials:
            return "bad credentials";
        case CommandDispatchReason::MissingArgument:
            return "missing argument";
        case CommandDispatchReason::Rejected:
            return "command rejected";
        case CommandDispatchReason::PolicyBlocked:
            return "policy blocked";
        case CommandDispatchReason::ControllerNotInitialized:
            return "bluetooth controller not started";
        case CommandDispatchReason::DiscoveryNotActive:
            return "discovery not active";
        case CommandDispatchReason::DiscoveryAlreadyActive:
            return "discovery already active";
        case CommandDispatchReason::NoActiveConnection:
            return "missing active connection handle";
        case CommandDispatchReason::InvalidVerb:
            return "invalid command verb";
        case CommandDispatchReason::WifiControlDisabled:
            return "wifi control is disabled";
        case CommandDispatchReason::Unauthorized:
            return "unauthorized";
        case CommandDispatchReason::NotFound:
            return "not found";
        case CommandDispatchReason::QueueFull:
            return "command queue full";
        case CommandDispatchReason::InternalError:
        default:
            return "internal error";
    }
}

const char *commandDispatchReasonToOpenApiDescription(CommandDispatchReason reason) {
    switch (reason) {
        case CommandDispatchReason::Ok:
            return "OK";
        case CommandDispatchReason::Queued:
            return "Accepted";
        case CommandDispatchReason::NotConnected:
            return "Wiimote not connected";
        case CommandDispatchReason::Locked:
            return "Forbidden";
        case CommandDispatchReason::UnknownCommand:
        case CommandDispatchReason::InvalidVerb:
        case CommandDispatchReason::BadArgument:
        case CommandDispatchReason::MissingArgument:
            return "Bad request";
        case CommandDispatchReason::BadCredentials:
        case CommandDispatchReason::Unauthorized:
            return "Unauthorized";
        case CommandDispatchReason::Rejected:
            return "Command rejected";
        case CommandDispatchReason::PolicyBlocked:
            return "Policy blocked";
        case CommandDispatchReason::ControllerNotInitialized:
            return "Bluetooth controller not started";
        case CommandDispatchReason::DiscoveryNotActive:
            return "Discovery not active";
        case CommandDispatchReason::DiscoveryAlreadyActive:
            return "Discovery already active";
        case CommandDispatchReason::NoActiveConnection:
            return "Missing active connection handle";
        case CommandDispatchReason::WifiControlDisabled:
            return "Wi-Fi control is disabled";
        case CommandDispatchReason::NotFound:
            return "Not found";
        case CommandDispatchReason::QueueFull:
            return "Command queue full";
        case CommandDispatchReason::InternalError:
        default:
            return "Internal error";
    }
}

int commandDispatchReasonToHttpStatus(CommandDispatchReason reason) {
    switch (reason) {
        case CommandDispatchReason::Ok:
            return 200;
        case CommandDispatchReason::Queued:
            return 202;
        case CommandDispatchReason::Unauthorized:
        case CommandDispatchReason::BadCredentials:
            return 401;
        case CommandDispatchReason::PolicyBlocked:
        case CommandDispatchReason::Locked:
            return 403;
        case CommandDispatchReason::BadArgument:
        case CommandDispatchReason::MissingArgument:
        case CommandDispatchReason::InvalidVerb:
        case CommandDispatchReason::UnknownCommand:
            return 400;
        case CommandDispatchReason::NotFound:
            return 404;
        case CommandDispatchReason::QueueFull:
            return 503;
        case CommandDispatchReason::InternalError:
            return 500;
        case CommandDispatchReason::NotConnected:
        case CommandDispatchReason::Rejected:
        case CommandDispatchReason::ControllerNotInitialized:
        case CommandDispatchReason::DiscoveryNotActive:
        case CommandDispatchReason::DiscoveryAlreadyActive:
        case CommandDispatchReason::NoActiveConnection:
        case CommandDispatchReason::WifiControlDisabled:
        default:
            return 409;
    }
}

bool commandDispatchReasonIsSuccess(CommandDispatchReason reason) {
    return reason == CommandDispatchReason::Ok || reason == CommandDispatchReason::Queued;
}