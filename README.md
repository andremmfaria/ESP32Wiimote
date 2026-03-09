# ESP32Wiimote

ESP32Wiimote is an Arduino library for ESP32 boards that connects to a Wii Remote (Wiimote) over Bluetooth Classic, with optional Nunchuk support.

## Features

- Button input (A/B/C/Z/1/2/Minus/Home/Plus/D-Pad)
- Wiimote accelerometer data
- Nunchuk accelerometer and analog stick data
- Connection state check via `isConnected()`
- Battery level readout via `getBatteryLevel()`

## Requirements

- ESP32 board (any)
- Arduino IDE `>= 1.8.5`
- ESP32 core package: `esp32:esp32@2.0.17`
- Wii Remote (RVL-CNT-01)
- Wii Nunchuk (optional)

## Installation

1. Download this repository as a `.zip`.
2. In Arduino IDE, go to `Sketch > Include Library > Add .ZIP Library...`.
3. Select the downloaded `.zip` file.

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

Use `getBatteryLevel()` to read the Wiimote battery level.

```cpp
uint8_t level = wiimote.getBatteryLevel();
float percent = (level / 255.0f) * 100.0f;

Serial.printf("Battery: %u (%.1f%%)\n", level, percent);
```

Notes:
- `getBatteryLevel()` returns a raw value from `0` to `255`.
- The value is updated from Wiimote status reports while connected.

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

## License

See [LICENSE.md](./LICENSE.md).
