#ifndef WEB_RESPONSE_SERIALIZER_H
#define WEB_RESPONSE_SERIALIZER_H

#include <cstddef>
#include <stdint.h>

/**
 * Web Response Serializer
 *
 * Emits bounded JSON responses for REST API endpoints.
 * All serializer functions write into a caller-supplied buffer and return
 * WebSerializeResult::Ok on success or WebSerializeResult::BufferTooSmall
 * if the output was truncated.
 *
 * Snapshot structs are hardware-free POD types so this module can be tested
 * in the native environment without any ESP32/Arduino dependencies.
 */

// ===== Result Enum =====

enum class WebSerializeResult {
    Ok,              // JSON written to buffer successfully
    BufferTooSmall,  // Buffer too small to hold the full output
};

// ===== Snapshot Types =====

/** Flat snapshot of the current Wiimote input (buttons + accelerometer + nunchuk). */
struct WebWiimoteInputSnapshot {
    uint32_t buttonMask;  ///< Raw button bitmask (ButtonState cast to uint32_t)
    uint8_t accelX;
    uint8_t accelY;
    uint8_t accelZ;
    uint8_t nunchukX;  ///< Nunchuk analog stick X axis (0 if no nunchuk)
    uint8_t nunchukY;  ///< Nunchuk analog stick Y axis (0 if no nunchuk)
};

/** Snapshot of high-level Wiimote connection status. */
struct WebWiimoteStatusSnapshot {
    bool connected;
    uint8_t batteryLevel;  ///< 0-100 percentage
};

/** Snapshot of the Bluetooth controller internal state. */
struct WebControllerStatusSnapshot {
    bool initialized;
    bool started;
    bool scanning;
    bool connected;
    uint16_t activeConnectionHandle;
    bool fastReconnectActive;
    bool autoReconnectEnabled;
};

/** Snapshot of the active ESP32WiimoteConfig values. */
struct WebConfigSnapshot {
    int nunchukStickThreshold;
    int txQueueSize;
    int rxQueueSize;
    uint32_t fastReconnectTtlMs;
};

/** Snapshot of Wi-Fi control lifecycle state. */
struct WebWifiControlStateSnapshot {
    bool enabled;
    bool ready;
    bool networkCredentialsConfigured;
    bool networkConnectAttempted;
    bool networkConnected;
    bool networkConnectFailed;
    bool restAndWebSocket;
    bool serverStarted;
    bool serverBindFailed;
    bool hasToken;
};

// ===== Serializer Interface =====

/**
 * Write a generic success response: {"status":"ok"}
 */
WebSerializeResult serializeOk(char *buf, size_t size);

/**
 * Write a generic error response: {"status":"error","message":"<msg>"}
 * @param message Error message string (must not be null)
 */
WebSerializeResult serializeError(char *buf, size_t size, const char *message);

/**
 * Write current Wiimote input snapshot as JSON.
 * Fields: buttons (hex string), accel{X,Y,Z}, nunchuk{X,Y}
 */
WebSerializeResult serializeWiimoteInput(char *buf, size_t size, const WebWiimoteInputSnapshot &in);

/**
 * Write Wiimote connection status as JSON.
 * Fields: connected (bool), batteryLevel (int)
 */
WebSerializeResult serializeWiimoteStatus(char *buf,
                                          size_t size,
                                          const WebWiimoteStatusSnapshot &st);

/**
 * Write detailed Bluetooth controller state as JSON.
 * Fields: initialized, started, scanning, connected, activeConnectionHandle,
 *         fastReconnectActive, autoReconnectEnabled
 */
WebSerializeResult serializeControllerStatus(char *buf,
                                             size_t size,
                                             const WebControllerStatusSnapshot &st);

/**
 * Write the active runtime config as JSON.
 * Fields: nunchukStickThreshold, txQueueSize, rxQueueSize, fastReconnectTtlMs
 */
WebSerializeResult serializeConfig(char *buf, size_t size, const WebConfigSnapshot &cfg);

/**
 * Write Wi-Fi control lifecycle state as JSON.
 */
WebSerializeResult serializeWifiControlState(char *buf,
                                             size_t size,
                                             const WebWifiControlStateSnapshot &state);

#endif  // WEB_RESPONSE_SERIALIZER_H
