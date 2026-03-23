#ifndef ESP32_WIIMOTE_SERIAL_COMMAND_DISPATCHER_H
#define ESP32_WIIMOTE_SERIAL_COMMAND_DISPATCHER_H

#include "serial_command_parser.h"

#include <stdint.h>

// ---------------------------------------------------------------------------
// Dispatch result
// ---------------------------------------------------------------------------

enum class SerialDispatchResult : uint8_t {
    Ok,               // Command accepted / queued
    NotConnected,     // Command requires a Wiimote connection
    UnknownCommand,   // Verb not recognised
    BadArgument,      // Argument was present but could not be parsed
    MissingArgument,  // Required argument was absent
    Rejected,         // Command accepted but rejected by implementation (guard)
};

// ---------------------------------------------------------------------------
// Abstract API target
// The dispatcher calls only the methods it needs through this interface.
// In production the real ESP32Wiimote adapter implements it; in tests a mock does.
// ---------------------------------------------------------------------------

struct SerialCommandTarget {
    // Wiimote output commands
    virtual bool setLeds(uint8_t ledMask) = 0;
    virtual bool setReportingMode(uint8_t mode, bool continuous) = 0;
    virtual bool setAccelerometerEnabled(bool enabled) = 0;
    virtual bool requestStatus() = 0;

    // Bluetooth controller commands
    virtual void setScanEnabled(bool enabled) = 0;
    virtual bool startDiscovery() = 0;
    virtual bool stopDiscovery() = 0;
    virtual bool disconnectActiveController(uint8_t reason) = 0;
    virtual void setAutoReconnectEnabled(bool enabled) = 0;
    virtual void clearReconnectCache() = 0;

    // Query
    virtual bool isConnected() const = 0;
    virtual uint8_t getBatteryLevel() const = 0;

    virtual ~SerialCommandTarget() = default;
};

// ---------------------------------------------------------------------------
// Dispatch command
// ---------------------------------------------------------------------------

// Dispatch a successfully parsed command to `target`.
// Returns a SerialDispatchResult that the caller can turn into a response line.
SerialDispatchResult serialCommandDispatch(const SerialParsedCommand &cmd,
                                           SerialCommandTarget *target);

// ---------------------------------------------------------------------------
// Argument parsing helpers (exposed for testing)
// ---------------------------------------------------------------------------

// Parse a boolean token ("on"/"off"/"true"/"false"/"1"/"0") into `out`.
// Returns true on success, false if the token is not a recognised boolean.
bool serialParseBool(const char *token, bool *out);

// Parse an integer token (decimal or "0x" hex) into `out`.
// Returns true on success, false if parsing fails.
bool serialParseUint8(const char *token, uint8_t *out);

// Parse a uint16_t token (decimal or "0x" hex) into `out`.
// Returns true on success, false if parsing fails.
bool serialParseUint16(const char *token, uint16_t *out);

#endif  // ESP32_WIIMOTE_SERIAL_COMMAND_DISPATCHER_H
