#include "command_dispatch_message_formatter.h"

#include <cstdio>

bool commandDispatchFormatMessage(CommandDispatchMessageTemplate tmpl,
                                  const char *field,
                                  char *out,
                                  size_t outSize) {
    if (field == nullptr || out == nullptr || outSize == 0U) {
        return false;
    }

    const char *format = nullptr;
    switch (tmpl) {
        case CommandDispatchMessageTemplate::MissingField:
            format = "missing field: %s";
            break;
        case CommandDispatchMessageTemplate::InvalidField:
            format = "invalid field: %s";
            break;
        case CommandDispatchMessageTemplate::EmptyField:
            format = "%s must not be empty";
            break;
        default:
            return false;
    }

    const int kWritten = std::snprintf(out, outSize, format, field);
    return kWritten >= 0 && static_cast<size_t>(kWritten) < outSize;
}

const char *commandDispatchMessageText(CommandDispatchMessageTemplate tmpl) {
    switch (tmpl) {
        case CommandDispatchMessageTemplate::MissingRequiredField:
            return "missing required field";
        case CommandDispatchMessageTemplate::InvalidRequestField:
            return "invalid request field";
        case CommandDispatchMessageTemplate::FieldMustNotBeEmpty:
            return "field must not be empty";
        case CommandDispatchMessageTemplate::ResponseBufferTooSmall:
            return "response buffer too small";
        case CommandDispatchMessageTemplate::InvalidCommandId:
            return "invalid command id";
        case CommandDispatchMessageTemplate::CommandNotFound:
            return "command not found";
        case CommandDispatchMessageTemplate::FailedSerializeWiimoteStatus:
            return "failed to serialize wiimote status";
        case CommandDispatchMessageTemplate::FailedSerializeRuntimeConfig:
            return "failed to serialize runtime config";
        case CommandDispatchMessageTemplate::FailedSerializeWifiControlState:
            return "failed to serialize wifi control state";
        case CommandDispatchMessageTemplate::MissingRequestBody:
            return "missing request body";
        case CommandDispatchMessageTemplate::RequestBodyTooLarge:
            return "request body too large";
        case CommandDispatchMessageTemplate::MalformedRequestBody:
            return "malformed request body";
        case CommandDispatchMessageTemplate::UnknownScanVerb:
            return "unknown command verb: expected scan_start or scan_stop";
        case CommandDispatchMessageTemplate::UnknownDiscoveryVerb:
            return "unknown command verb: expected discovery_start or discovery_stop";
        case CommandDispatchMessageTemplate::InvalidWifiMode:
            return "invalid field: mode (expected rest or rest-ws)";
        case CommandDispatchMessageTemplate::WifiApiTokenMutationDisabledByPolicy:
            return "wifi API token mutation is disabled by policy";
        case CommandDispatchMessageTemplate::MissingField:
        case CommandDispatchMessageTemplate::InvalidField:
        case CommandDispatchMessageTemplate::EmptyField:
        default:
            return "internal error";
    }
}
