#include "serial_response_formatter.h"

#include <cstdarg>
#include <cstdio>

static size_t serialFormatLine(char *out, size_t outSize, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int kWritten = vsnprintf(out, outSize, format, args);
    va_end(args);

    if (kWritten < 0) {
        if (outSize > 0U) {
            out[0] = '\0';
        }
        return 0U;
    }
    return static_cast<size_t>(kWritten);
}

size_t serialFormatOk(char *out, size_t outSize) {
    return serialFormatLine(out, outSize, "@wm: ok");
}

size_t serialFormatOkQueued(char *out, size_t outSize) {
    return serialFormatLine(out, outSize, "@wm: ok queued");
}

size_t serialFormatError(char *out, size_t outSize, const char *code) {
    return serialFormatLine(out, outSize, "@wm: error %s", code);
}

size_t serialFormatDispatchResult(char *out, size_t outSize, SerialDispatchResult result) {
    switch (result) {
        case SerialDispatchResult::Ok:
            return serialFormatOk(out, outSize);
        case SerialDispatchResult::NotConnected:
            return serialFormatError(out, outSize, "not_connected");
        case SerialDispatchResult::UnknownCommand:
            return serialFormatError(out, outSize, "unknown_command");
        case SerialDispatchResult::BadArgument:
            return serialFormatError(out, outSize, "bad_argument");
        case SerialDispatchResult::MissingArgument:
            return serialFormatError(out, outSize, "missing_argument");
        case SerialDispatchResult::Rejected:
            return serialFormatError(out, outSize, "rejected");
        default:
            return serialFormatError(out, outSize, "internal");
    }
}

size_t serialFormatParseResult(char *out, size_t outSize, SerialParseResult result) {
    switch (result) {
        case SerialParseResult::Ok:
            return serialFormatOk(out, outSize);
        case SerialParseResult::EmptyLine:
            return serialFormatError(out, outSize, "empty_line");
        case SerialParseResult::NotACommand:
            return serialFormatError(out, outSize, "not_a_command");
        case SerialParseResult::LineTooLong:
            return serialFormatError(out, outSize, "line_too_long");
        case SerialParseResult::TooManyTokens:
            return serialFormatError(out, outSize, "too_many_tokens");
        default:
            return serialFormatError(out, outSize, "internal");
    }
}

size_t serialFormatStatus(char *out,
                          size_t outSize,
                          bool connected,
                          bool scanning,
                          uint8_t reportingMode,
                          bool accelerometerEnabled,
                          uint8_t batteryLevel) {
    return serialFormatLine(
        out, outSize, "@wm: status connected=%u scan=%u mode=0x%02X accel=%u battery=%u",
        connected ? 1U : 0U, scanning ? 1U : 0U, static_cast<unsigned int>(reportingMode),
        accelerometerEnabled ? 1U : 0U, static_cast<unsigned int>(batteryLevel));
}

size_t serialFormatConfig(char *out,
                          size_t outSize,
                          bool autoReconnectEnabled,
                          uint32_t fastReconnectTtlMs,
                          bool serialControlEnabled) {
    return serialFormatLine(
        out, outSize, "@wm: cfg auto_reconnect=%u fast_reconnect_ttl_ms=%lu serial_control=%u",
        autoReconnectEnabled ? 1U : 0U, static_cast<unsigned long>(fastReconnectTtlMs),
        serialControlEnabled ? 1U : 0U);
}
