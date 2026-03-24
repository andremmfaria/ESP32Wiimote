#ifndef ESP32_WIIMOTE_SERIAL_RESPONSE_FORMATTER_H
#define ESP32_WIIMOTE_SERIAL_RESPONSE_FORMATTER_H

#include "serial_command_dispatcher.h"
#include "serial_command_parser.h"

#include <stddef.h>
#include <stdint.h>

// Format "@wm: ok"
size_t serialFormatOk(char *out, size_t outSize);

// Format "@wm: ok queued"
size_t serialFormatOkQueued(char *out, size_t outSize);

// Format "@wm: error <code>"
size_t serialFormatError(char *out, size_t outSize, const char *code);

// Format a dispatcher result into a stable response line.
size_t serialFormatDispatchResult(char *out, size_t outSize, SerialDispatchResult result);

// Format a parser result into a stable response line.
size_t serialFormatParseResult(char *out, size_t outSize, SerialParseResult result);

// Format "@wm: status connected=<0|1> scan=<0|1> mode=0xNN accel=<0|1> battery=<0-100>"
size_t serialFormatStatus(char *out,
                          size_t outSize,
                          bool connected,
                          bool scanning,
                          uint8_t reportingMode,
                          bool accelerometerEnabled,
                          uint8_t batteryLevel);

// Format "@wm: cfg auto_reconnect=<0|1> fast_reconnect_ttl_ms=<N> serial_control=<0|1>"
size_t serialFormatConfig(char *out,
                          size_t outSize,
                          bool autoReconnectEnabled,
                          uint32_t fastReconnectTtlMs,
                          bool serialControlEnabled);

// Format "@wm: wifi enabled=<0|1> ready=<0|1> connected=<0|1> failed=<0|1> mode=<rest|rest-ws>"
size_t serialFormatWifiStatus(char *out,
                              size_t outSize,
                              bool enabled,
                              bool ready,
                              bool connected,
                              bool failed,
                              bool restAndWebSocket);

#endif  // ESP32_WIIMOTE_SERIAL_RESPONSE_FORMATTER_H
