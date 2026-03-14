# ESP32Wiimote

ESP32Wiimote is an Arduino library for ESP32 boards that connects to a Wii Remote (Wiimote) over Bluetooth Classic, with optional Nunchuk support.

Due to ESP32 Bluetooth Classic HCI limitations, this project supports one active Wiimote connection per ESP32 radio.

## Features

- ✅ Button input (A/B/C/Z/1/2/Minus/Home/Plus/D-Pad)
- ✅ Wiimote accelerometer data
- ✅ Nunchuk accelerometer and analog stick data
- ✅ Connection state check via `isConnected()`
- ✅ Single-controller operation (1 Wiimote per ESP32 radio, due to Bluetooth Classic HCI limits)
- ✅ Battery level readout (0-100%) via `getBatteryLevel()`
- ✅ Battery status requests via `requestBatteryUpdate()`
- ✅ Comprehensive 4-level logging system (ERROR/WARN/INFO/DEBUG)
- ✅ Unit tests with PlatformIO
- ✅ Hardware integration tests

## Documentation

- 📖 **[API Reference](docs/API.md)** - Complete API documentation with examples
- 🔧 **[Testing Guide](docs/TESTING.md)** - Run unit tests and integration tests
- 📊 **[Logging System](docs/LOGGING.md)** - Configure debug output (4 levels)
- 🏗️ **[Architecture](docs/ARCHITECTURE.md)** - System design and data flow
- 🔍 **[Troubleshooting](docs/TROUBLESHOOTING.md)** - Common issues and solutions

## Requirements

- ESP32 board (any)
- Arduino CLI `>= 1.4.1`
- ESP32 core package: `esp32:esp32@3.3.7`
- Wii Remote (RVL-CNT-01)
- Wii Nunchuk (optional)

## Installation

1. Download this repository as a `.zip`.
2. In Arduino IDE, go to `Sketch > Include Library > Add .ZIP Library...`.
3. Select the downloaded `.zip` file.

## Quick Start

```cpp
#include "ESP32Wiimote.h"

ESP32Wiimote wiimote;

void setup() {
    Serial.begin(115200);
    wiimote.init();
}

void loop() {
    wiimote.task();  // Must be called regularly
    
    // Check for new data
    if (wiimote.available()) {
        ButtonState btn = wiimote.getButtonState();
        
        if (btn == BUTTON_A) {
            Serial.println("A button pressed!");
        }
        
        // Read sensors
        AccelState accel = wiimote.getAccelState();
        Serial.printf("Accel: %d, %d, %d\n", 
            accel.xAxis, accel.yAxis, accel.zAxis);
    }
}
```

For complete examples, see [API Reference](docs/API.md).

## Example

See: [ESP32WiimoteDemo.ino](./examples/ESP32WiimoteDemo/ESP32WiimoteDemo.ino)

The demo now covers all available features:

- connection state changes
- periodic battery reporting
- button decoding
- Wiimote and Nunchuk accelerometer output
- Nunchuk stick output
- update-rate statistics
- optional filter configuration

## Usage

No manual Bluetooth pairing is required.

Connection model note: due to Bluetooth Classic HCI limitations, this library is designed for a single Wiimote connected to one ESP32 radio at a time.

1. Press `1 + 2` on the Wii Remote.
2. LED1 turns on when the connection is established.

![Wiimote LED1 on](./resources/remocon_led1_on.png)

## Connection Test

Use `isConnected()` to check connection status at runtime.

```cpp
#include "ESP32Wiimote.h"

ESP32Wiimote wiimote;

void setup() {
  Serial.begin(115200);
  wiimote.init();
}

void loop() {
  wiimote.task();

  if (wiimote.isConnected()) {
    Serial.println("Wiimote connected");
  } else {
    Serial.println("Wiimote disconnected");
  }

  delay(1000);
}
```

For less serial spam, print only on state change:

```cpp
static bool wasConnected = false;

void loop() {
  wiimote.task();
  bool connected = wiimote.isConnected();

  if (connected != wasConnected) {
    Serial.println(connected ? "CONNECTED" : "DISCONNECTED");
    wasConnected = connected;
  }
}
```

## Battery Level

Use `getBatteryLevel()` to read the Wiimote battery level (percentage).

```cpp
uint8_t battery = wiimote.getBatteryLevel();
Serial.printf("Battery: %d%%\n", battery);
```

**Manual Updates:**

Request a battery status refresh:

```cpp
wiimote.requestBatteryUpdate();  // Async - check after delay
delay(100);
uint8_t battery = wiimote.getBatteryLevel();
```

**Auto-Update:**

Battery level is automatically requested when the Wiimote first connects.

**Notes:**

- Returns 0-100 (percentage)
- Updated from Wiimote status reports (0x20)
- `requestBatteryUpdate()` only works when connected

See [API Reference](docs/API.md#battery-management) for details.

## Filters

You can reduce update volume by ignoring selected categories:

```cpp
wiimote.addFilter(ACTION_IGNORE, FILTER_ACCEL);
wiimote.addFilter(ACTION_IGNORE, FILTER_NUNCHUK_STICK);
wiimote.addFilter(ACTION_IGNORE, FILTER_BUTTON);
```

Available filters:

- `FILTER_ACCEL`
- `FILTER_NUNCHUK_STICK`
- `FILTER_BUTTON`

See [API Reference](docs/API.md#filtering) for details.

## Testing

ESP32Wiimote includes comprehensive unit tests and integration tests.

### Run Native Tests (No Hardware)

```bash
pio test -e native
```

Fast unit tests run on your PC in ~1.5 seconds. No ESP32 required!

### Run Integration Tests (ESP32 + Wiimote)

```bash
pio test -e esp32dev --upload-port /dev/ttyUSB0 -v
```

Hardware tests with a real Wiimote. The `-v` flag shows test action prompts.

See [Testing Guide](docs/TESTING.md) for complete instructions.

## Troubleshooting

### Connection Issues

**Wiimote won't connect?**

- Press and hold 1 + 2 buttons simultaneously
- Wait for LEDs to blink
- Keep ESP32 within 5 meters

**Battery shows 0%?**

- Call `wiimote.requestBatteryUpdate()` after connection
- Battery updates asynchronously - check after 100ms

**Buttons not responding?**

- Always check `wiimote.available()` before reading data
- Make sure `wiimote.task()` is called in `loop()`

See [Troubleshooting Guide](docs/TROUBLESHOOTING.md) for more solutions.

## Logging

Control debug output by defining `WIIMOTE_VERBOSE` before including the library:

```cpp
#define WIIMOTE_VERBOSE 2

#include "ESP32Wiimote.h"
```

You can also set it from PlatformIO:

```ini
build_flags = -DWIIMOTE_VERBOSE=2
```

If you do not define it, the library defaults to:

```cpp
#define WIIMOTE_VERBOSE 2  // 0=Errors, 1=+Warnings, 2=+Info, 3=+Debug
```

**Log Levels:**

- **0**: Errors only (production)
- **1**: + Warnings
- **2**: + Info messages (default - shows connection events)
- **3**: + Debug traces (packet dumps, detailed flow)

See [Logging System](docs/LOGGING.md) for complete documentation.

## Contributing

Contributions are welcome! Whether it's bug fixes, new features, or documentation improvements.

### Quick Start for Contributors

1. Fork and clone the repository
2. Run tests: `pio test -e native`
3. Make your changes
4. Add tests for new features
5. Submit a pull request

See [Contributing Guide](docs/CONTRIBUTING.md) for detailed instructions on:

- Development setup
- Code organization
- Coding standards
- Testing requirements
- Pull request process

## License

See [LICENSE.md](./LICENSE.md).
