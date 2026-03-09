# ESP32Wiimote

ESP32Wiimote is an Arduino library for ESP32 boards that connects to a Wii Remote (Wiimote) over Bluetooth Classic, with optional Nunchuk support.

## Features

- Button input (A/B/C/Z/1/2/Minus/Home/Plus/D-Pad)
- Wiimote accelerometer data
- Nunchuk accelerometer and analog stick data
- Connection state check via `isConnected()`

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

Notes:

- Accelerometer output can be verbose.
- You can reduce events with `wiimote.addFilter(ACTION_IGNORE, FILTER_ACCEL)`.

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

## License

See [LICENSE.md](./LICENSE.md).
