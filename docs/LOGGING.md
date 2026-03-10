# Logging System

ESP32Wiimote uses a 4-level runtime logging system.

## Quick Start

Set the log level explicitly in your sketch:

```cpp
#include "ESP32Wiimote.h"

ESP32Wiimote wiimote;

void setup() {
    Serial.begin(115200);
    ESP32Wiimote::setLogLevel(WIIMOTE_LOG_WARNING);
    wiimote.init();
}
```

## Why Runtime Configuration

`#define WIIMOTE_VERBOSE ...` inside a sketch is not a reliable way to control
logs emitted by library `.cpp` files. Sketch-local defines only affect the
sketch translation unit, while the library is compiled separately.

Use `ESP32Wiimote::setLogLevel(...)` for consistent behavior in:

- Arduino IDE
- arduino-cli
- PlatformIO

## Log Levels

- `WIIMOTE_LOG_ERROR` (`0`): critical errors only
- `WIIMOTE_LOG_WARNING` (`1`): errors + warnings
- `WIIMOTE_LOG_INFO` (`2`): errors + warnings + informational logs
- `WIIMOTE_LOG_DEBUG` (`3`): all logs including detailed debug traces

Recommended default for user-facing serial integrations:

```cpp
ESP32Wiimote::setLogLevel(WIIMOTE_LOG_WARNING);
```

## API Reference

```cpp
static void ESP32Wiimote::setLogLevel(uint8_t level);
static uint8_t ESP32Wiimote::getLogLevel(void);
```

Example:

```cpp
ESP32Wiimote::setLogLevel(WIIMOTE_LOG_DEBUG);
uint8_t level = ESP32Wiimote::getLogLevel();
```

## Advanced: Build Flags Compatibility

The library still accepts `WIIMOTE_VERBOSE` as an optional *initial default* when
provided as a global compiler define (for example with PlatformIO build flags).

```ini
build_flags = -DWIIMOTE_VERBOSE=2
```

Runtime API calls always override the initial value.

## Log Macros

The internal logging macros remain available for library code:

```cpp
LOG_ERROR("Critical: %s\n", msg);
LOG_WARN("Potential issue\n");
LOG_INFO("Connected\n");
LOG_DEBUG("Packet: type=0x%02x len=%d\n", type, len);
```

They are runtime-gated using the currently configured log level.
