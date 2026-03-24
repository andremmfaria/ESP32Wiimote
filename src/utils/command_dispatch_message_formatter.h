#ifndef ESP32_WIIMOTE_COMMAND_DISPATCH_MESSAGE_FORMATTER_H
#define ESP32_WIIMOTE_COMMAND_DISPATCH_MESSAGE_FORMATTER_H

#include <stddef.h>
#include <stdint.h>

enum class CommandDispatchMessageTemplate : uint8_t {
    MissingField,
    InvalidField,
    EmptyField,

    MissingRequiredField,
    InvalidRequestField,
    FieldMustNotBeEmpty,

    ResponseBufferTooSmall,
    InvalidCommandId,
    CommandNotFound,

    FailedSerializeWiimoteStatus,
    FailedSerializeRuntimeConfig,
    FailedSerializeWifiControlState,

    MissingRequestBody,
    RequestBodyTooLarge,
    MalformedRequestBody,

    UnknownScanVerb,
    UnknownDiscoveryVerb,
    InvalidWifiMode,

    WifiApiTokenMutationDisabledByPolicy,
};

// Format a field-aware message template into `out`.
// Returns true when formatting succeeds and the output fits in `outSize`.
bool commandDispatchFormatMessage(CommandDispatchMessageTemplate tmpl,
                                  const char *field,
                                  char *out,
                                  size_t outSize);

// Return static text for non-parameterized templates.
const char *commandDispatchMessageText(CommandDispatchMessageTemplate tmpl);

#endif  // ESP32_WIIMOTE_COMMAND_DISPATCH_MESSAGE_FORMATTER_H
