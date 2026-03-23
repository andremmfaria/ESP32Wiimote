#include "serial_command_dispatcher.h"

#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------------
// Argument parsing helpers
// ---------------------------------------------------------------------------

bool serialParseBool(const char *token, bool *out) {
    if (strcmp(token, "on") == 0 || strcmp(token, "true") == 0 || strcmp(token, "1") == 0) {
        *out = true;
        return true;
    }
    if (strcmp(token, "off") == 0 || strcmp(token, "false") == 0 || strcmp(token, "0") == 0) {
        *out = false;
        return true;
    }
    return false;
}

bool serialParseUint8(const char *token, uint8_t *out) {
    if (token == nullptr || token[0] == '\0') {
        return false;
    }
    char *end = nullptr;
    const int kBase = (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) ? 16 : 10;
    const long kVal = strtol(token, &end, kBase);
    if (end == token || *end != '\0') {
        return false;
    }
    if (kVal < 0 || kVal > 255) {
        return false;
    }
    *out = static_cast<uint8_t>(kVal);
    return true;
}

bool serialParseUint16(const char *token, uint16_t *out) {
    if (token == nullptr || token[0] == '\0') {
        return false;
    }
    char *end = nullptr;
    const int kBase = (token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) ? 16 : 10;
    const long kVal = strtol(token, &end, kBase);
    if (end == token || *end != '\0') {
        return false;
    }
    if (kVal < 0 || kVal > 65535) {
        return false;
    }
    *out = static_cast<uint16_t>(kVal);
    return true;
}

// ---------------------------------------------------------------------------
// Command handlers
// ---------------------------------------------------------------------------

// wm status
static SerialDispatchResult handleStatus(const SerialParsedCommand & /*cmd*/,
                                         SerialCommandTarget *target) {
    (void)target;
    return SerialDispatchResult::Ok;
}

// wm led <mask>
static SerialDispatchResult handleLed(const SerialParsedCommand &cmd, SerialCommandTarget *target) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    uint8_t mask = 0;
    if (!serialParseUint8(cmd.tokens[2], &mask)) {
        return SerialDispatchResult::BadArgument;
    }
    if (!target->isConnected()) {
        return SerialDispatchResult::NotConnected;
    }
    return target->setLeds(mask) ? SerialDispatchResult::Ok : SerialDispatchResult::Rejected;
}

// wm mode <value> [continuous]
static SerialDispatchResult handleMode(const SerialParsedCommand &cmd,
                                       SerialCommandTarget *target) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    uint8_t mode = 0;
    if (!serialParseUint8(cmd.tokens[2], &mode)) {
        return SerialDispatchResult::BadArgument;
    }
    bool continuous = false;
    if (cmd.tokenCount >= 4) {
        if (!serialParseBool(cmd.tokens[3], &continuous)) {
            return SerialDispatchResult::BadArgument;
        }
    }
    if (!target->isConnected()) {
        return SerialDispatchResult::NotConnected;
    }
    return target->setReportingMode(mode, continuous) ? SerialDispatchResult::Ok
                                                      : SerialDispatchResult::Rejected;
}

// wm accel <on|off>
static SerialDispatchResult handleAccel(const SerialParsedCommand &cmd,
                                        SerialCommandTarget *target) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    bool enabled = false;
    if (!serialParseBool(cmd.tokens[2], &enabled)) {
        return SerialDispatchResult::BadArgument;
    }
    return target->setAccelerometerEnabled(enabled) ? SerialDispatchResult::Ok
                                                    : SerialDispatchResult::Rejected;
}

// wm request-status
static SerialDispatchResult handleRequestStatus(const SerialParsedCommand & /*cmd*/,
                                                SerialCommandTarget *target) {
    if (!target->isConnected()) {
        return SerialDispatchResult::NotConnected;
    }
    return target->requestStatus() ? SerialDispatchResult::Ok : SerialDispatchResult::Rejected;
}

// wm scan <on|off>
static SerialDispatchResult handleScan(const SerialParsedCommand &cmd,
                                       SerialCommandTarget *target) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    bool enabled = false;
    if (!serialParseBool(cmd.tokens[2], &enabled)) {
        return SerialDispatchResult::BadArgument;
    }
    target->setScanEnabled(enabled);
    return SerialDispatchResult::Ok;
}

// wm discover <start|stop>
static SerialDispatchResult handleDiscover(const SerialParsedCommand &cmd,
                                           SerialCommandTarget *target) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    if (strcmp(cmd.tokens[2], "start") == 0) {
        return target->startDiscovery() ? SerialDispatchResult::Ok : SerialDispatchResult::Rejected;
    }
    if (strcmp(cmd.tokens[2], "stop") == 0) {
        return target->stopDiscovery() ? SerialDispatchResult::Ok : SerialDispatchResult::Rejected;
    }
    return SerialDispatchResult::BadArgument;
}

// wm disconnect [reason]
static SerialDispatchResult handleDisconnect(const SerialParsedCommand &cmd,
                                             SerialCommandTarget *target) {
    uint8_t reason = 0x16;  // LocalHostTerminated default
    if (cmd.tokenCount >= 3) {
        if (!serialParseUint8(cmd.tokens[2], &reason)) {
            return SerialDispatchResult::BadArgument;
        }
    }
    if (!target->isConnected()) {
        return SerialDispatchResult::NotConnected;
    }
    return target->disconnectActiveController(reason) ? SerialDispatchResult::Ok
                                                      : SerialDispatchResult::Rejected;
}

// wm reconnect <on|off|clear>
static SerialDispatchResult handleReconnect(const SerialParsedCommand &cmd,
                                            SerialCommandTarget *target) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    if (strcmp(cmd.tokens[2], "clear") == 0) {
        target->clearReconnectCache();
        return SerialDispatchResult::Ok;
    }
    bool enabled = false;
    if (!serialParseBool(cmd.tokens[2], &enabled)) {
        return SerialDispatchResult::BadArgument;
    }
    target->setAutoReconnectEnabled(enabled);
    return SerialDispatchResult::Ok;
}

// wm unlock <seconds>
static SerialDispatchResult handleUnlock(const SerialParsedCommand &cmd,
                                         SerialCommandSession *session,
                                         uint32_t nowMs) {
    if (cmd.tokenCount < 3) {
        return SerialDispatchResult::MissingArgument;
    }
    if (session == nullptr) {
        return SerialDispatchResult::Rejected;
    }

    uint16_t seconds = 0;
    if (!serialParseUint16(cmd.tokens[2], &seconds)) {
        return SerialDispatchResult::BadArgument;
    }

    const uint32_t kDurationMs = static_cast<uint32_t>(seconds) * 1000U;
    session->unlock(kDurationMs, nowMs);
    return SerialDispatchResult::Ok;
}

static bool serialCommandIsPrivileged(const char *verb) {
    return strcmp(verb, "led") == 0 || strcmp(verb, "mode") == 0 || strcmp(verb, "accel") == 0 ||
           strcmp(verb, "scan") == 0 || strcmp(verb, "discover") == 0 ||
           strcmp(verb, "disconnect") == 0 || strcmp(verb, "reconnect") == 0;
}

// ---------------------------------------------------------------------------
// Dispatch table entry
// ---------------------------------------------------------------------------

struct DispatchEntry {
    const char *verb;
    SerialDispatchResult (*handler)(const SerialParsedCommand &, SerialCommandTarget *);
};

static const DispatchEntry kDispatchTable[] = {
    {"status", handleStatus},
    {"led", handleLed},
    {"mode", handleMode},
    {"accel", handleAccel},
    {"request-status", handleRequestStatus},
    {"scan", handleScan},
    {"discover", handleDiscover},
    {"disconnect", handleDisconnect},
    {"reconnect", handleReconnect},
};

// ---------------------------------------------------------------------------
// Public dispatch entry point
// ---------------------------------------------------------------------------

SerialDispatchResult serialCommandDispatch(const SerialParsedCommand &cmd,
                                           SerialCommandTarget *target) {
    const SerialDispatchOptions kDefaultOptions = {nullptr, false, 0U};
    return serialCommandDispatch(cmd, target, kDefaultOptions);
}

SerialDispatchResult serialCommandDispatch(const SerialParsedCommand &cmd,
                                           SerialCommandTarget *target,
                                           const SerialDispatchOptions &options) {
    // cmd.tokens[0] == "wm" (guaranteed by the parser)
    // cmd.tokens[1] is the verb
    if (cmd.tokenCount < 2) {
        return SerialDispatchResult::UnknownCommand;
    }
    const char *verb = cmd.tokens[1];

    if (strcmp(verb, "unlock") == 0) {
        return handleUnlock(cmd, options.session, options.nowMs);
    }

    if (options.privilegedCommandsRequireUnlock && serialCommandIsPrivileged(verb)) {
        if (options.session == nullptr || !options.session->isUnlocked(options.nowMs)) {
            return SerialDispatchResult::Locked;
        }
    }

    for (const auto &entry : kDispatchTable) {
        if (strcmp(entry.verb, verb) == 0) {
            return entry.handler(cmd, target);
        }
    }
    return SerialDispatchResult::UnknownCommand;
}
