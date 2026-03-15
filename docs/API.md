# ESP32Wiimote API Reference

Complete API documentation for the ESP32Wiimote library.

## Table of Contents

- [ESP32Wiimote Class](#esp32wiimote-class)
- [Button States](#button-states)
- [Sensor Data Structures](#sensor-data-structures)
- [Filter Configuration](#filter-configuration)

---

## ESP32Wiimote Class

### Constructor

```cpp
ESP32Wiimote()
ESP32Wiimote(const ESP32WiimoteConfig &config)
```

Creates a new ESP32Wiimote instance.

Use the default constructor when you do not need to override any settings.

```cpp
ESP32Wiimote wiimote;
```

### Configuration Object

```cpp
struct ESP32WiimoteConfig {
    int nunchukStickThreshold = 1;
    int txQueueSize = 32;
    int rxQueueSize = 32;
    uint32_t fastReconnectTtlMs = 180000;
};
```

Use the config object when you want to override defaults.

```cpp
ESP32WiimoteConfig cfg;
cfg.nunchukStickThreshold = 5;
cfg.fastReconnectTtlMs = 180000;  // 3 minutes

ESP32Wiimote wiimote(cfg);
```

Set `fastReconnectTtlMs = 0` to disable fast reconnect and always use inquiry after disconnect.

---

### Initialization & Task Management

#### `void init()`

Initializes Bluetooth controller and HCI interface. Must be called once in `setup()`.

**Example:**

```cpp
void setup() {
    Serial.begin(115200);
    wiimote.init();
}
```

**Notes:**

- Initializes Bluetooth in non-discoverable mode
- Starts HCI packet queues
- Registers VHCI callbacks
- Supports one active Wiimote connection per ESP32 radio due to Bluetooth Classic HCI limitations
- Must be called before any other methods

---

#### `void task()`

Processes HCI packet queues. Must be called regularly in `loop()`.

**Example:**

```cpp
void loop() {
    wiimote.task();
    // ... your code ...
}
```

**Notes:**

- Handles incoming/outgoing Bluetooth packets
- Should be called as frequently as possible
- Non-blocking operation

---

### Connection Status

#### `bool isConnected()`

Checks if Wiimote is currently connected.

**Returns:** `true` if connected, `false` otherwise

**Connection model:**

- Single-controller design: one active Wiimote per ESP32 radio due to Bluetooth Classic HCI limitations

**Reconnect behavior:**

- After an unexpected disconnect, the stack attempts a fast reconnect to the last connected controller for up to 3 minutes
- If fast reconnect fails, it falls back to normal inquiry and can connect to another compatible controller
- When a different controller is found and connected, the previous fast-reconnect cache is cleared and replaced immediately

**Example:**

```cpp
if (wiimote.isConnected()) {
    Serial.println("Wiimote ready");
}
```

---

### Data Availability

#### `int available()`

Checks if new sensor or button data is available.

**Returns:**

- `1` if data has changed since last check
- `0` if no new data

**Example:**

```cpp
void loop() {
    wiimote.task();
    
    if (wiimote.available()) {
        // Process new data
        ButtonState btn = wiimote.getButtonState();
    }
}
```

**Notes:**

- Automatically parses incoming HCI reports
- Resets change flags after reading
- Filters are applied before returning

---

### Button Input

#### `ButtonState getButtonState()`

Gets current button state.

**Returns:** `ButtonState` enum value (see [Button States](#button-states))

**Example:**

```cpp
ButtonState btn = wiimote.getButtonState();

if (btn == BUTTON_A) {
    Serial.println("A button pressed");
}

// Check combinations
if (btn & BUTTON_A) {
    Serial.println("A is pressed (maybe with others)");
}
```

---

### Sensor Data

#### `AccelState getAccelState()`

Gets current Wiimote accelerometer state.

**Returns:** `AccelState` struct with x, y, z values (0-255)

**Example:**

```cpp
AccelState accel = wiimote.getAccelState();

Serial.printf("Accel: X=%d Y=%d Z=%d\n", 
    accel.xAxis, accel.yAxis, accel.zAxis);
```

**Notes:**

- Values are raw 8-bit accelerometer readings
- Typical rest position: ~128 for each axis
- Requires accelerometer mode enabled (see demo)

---

#### `NunchukState getNunchukState()`

Gets current Nunchuk state (if connected).

**Returns:** `NunchukState` struct with stick position, accelerometer, and buttons

**Example:**

```cpp
NunchukState nunchuk = wiimote.getNunchukState();

Serial.printf("Stick: X=%d Y=%d\n", 
    nunchuk.xStick, nunchuk.yStick);
Serial.printf("Accel: X=%d Y=%d Z=%d\n",
    nunchuk.xAccel, nunchuk.yAccel, nunchuk.zAccel);

if (nunchuk.cBtn) Serial.println("C button");
if (nunchuk.zBtn) Serial.println("Z button");
```

**Notes:**

- Returns default values if Nunchuk not connected
- Stick: 0-255 (center ~128)
- Accelerometer: 0-255 (rest ~128)

---

### Battery Management

#### `uint8_t getBatteryLevel()`

Gets battery level percentage.

**Returns:** Battery level in range 0-100 (percentage)

**Example:**

```cpp
uint8_t battery = wiimote.getBatteryLevel();
Serial.printf("Battery: %d%%\n", battery);
```

**Notes:**

- Returns 0-100 (percentage, not raw value)
- Updated automatically when Wiimote connects
- Call `requestBatteryUpdate()` for manual refresh

---

#### `void requestBatteryUpdate()`

Requests a battery status update from the Wiimote.

**Example:**

```cpp
// Refresh battery level every 60 seconds
static unsigned long lastUpdate = 0;
if (millis() - lastUpdate > 60000) {
    wiimote.requestBatteryUpdate();
    lastUpdate = millis();
}
```

**Notes:**

- Asynchronous - battery value updates when response is received
- Only works when connected
- Battery is automatically requested on initial connection

---

### Filtering

#### `void addFilter(int action, int filter)`

Configures data filters to reduce update frequency.

**Parameters:**

- `action` - Filter action (currently only `ACTION_IGNORE`)
- `filter` - Data type to filter

**Available Filters:**

- `FILTER_BUTTON` - Ignore button changes
- `FILTER_ACCEL` - Ignore Wiimote accelerometer changes
- `FILTER_NUNCHUK_STICK` - Ignore Nunchuk stick changes

**Example:**

```cpp
void setup() {
    wiimote.init();
    
    // Only care about buttons, ignore motion
    wiimote.addFilter(ACTION_IGNORE, FILTER_ACCEL);
    wiimote.addFilter(ACTION_IGNORE, FILTER_NUNCHUK_STICK);
}
```

**Notes:**

- Filters are applied in `available()`
- Can be combined (multiple filters active)
- Does not affect raw data reading

---

## Button States

```cpp
enum ButtonState {
    BUTTON_NONE           = 0x0000,
    
    // D-Pad
    BUTTON_LEFT           = 0x0001,
    BUTTON_RIGHT          = 0x0002,
    BUTTON_DOWN           = 0x0004,
    BUTTON_UP             = 0x0008,
    
    // Face buttons
    BUTTON_PLUS           = 0x0010,
    
    // System buttons
    BUTTON_TWO            = 0x0100,
    BUTTON_ONE            = 0x0200,
    BUTTON_B              = 0x0400,
    BUTTON_A              = 0x0800,
    BUTTON_MINUS          = 0x1000,
    BUTTON_HOME           = 0x8000,
    
    // Nunchuk
    BUTTON_Z              = 0x0001,  // Via Nunchuk
    BUTTON_C              = 0x0002,  // Via Nunchuk
};
```

### Checking Button States

**Single Button:**

```cpp
if (btn == BUTTON_A) {
    // Only A pressed
}
```

**Button Combinations:**

```cpp
if ((btn & BUTTON_A) && (btn & BUTTON_B)) {
    // A and B pressed together
}
```

**Any of Multiple:**

```cpp
if (btn & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) {
    // Any D-Pad pressed
}
```

---

## Sensor Data Structures

### AccelState

```cpp
struct AccelState {
    uint8_t xAxis;  // 0-255, ~128 at rest
    uint8_t yAxis;  // 0-255, ~128 at rest
    uint8_t zAxis;  // 0-255, ~128 at rest
};
```

### NunchukState

```cpp
struct NunchukState {
    uint8_t xStick;   // 0-255, ~128 centered
    uint8_t yStick;   // 0-255, ~128 centered
    uint8_t xAccel;   // 0-255, ~128 at rest
    uint8_t yAccel;   // 0-255, ~128 at rest
    uint8_t zAccel;   // 0-255, ~128 at rest
    bool cBtn;        // C button state
    bool zBtn;        // Z button state
};
```

---

## Filter Configuration

### FILTER_BUTTON

Ignores button state changes. `available()` returns 0 even when buttons change.

**Use case:** Reading only motion data

### FILTER_ACCEL

Ignores Wiimote accelerometer changes.

**Use case:** Buttons-only gameplay

### FILTER_NUNCHUK_STICK

Ignores Nunchuk analog stick position changes.

**Use case:** Using only Nunchuk buttons, not stick

---

## Complete Example

```cpp
#include "ESP32Wiimote.h"

ESP32Wiimote wiimote;

void setup() {
    Serial.begin(115200);
    wiimote.init();
}

void loop() {
    wiimote.task();
    
    // Check connection
    if (!wiimote.isConnected()) {
        delay(100);
        return;
    }
    
    // Process data when available
    if (wiimote.available()) {
        // Buttons
        ButtonState btn = wiimote.getButtonState();
        if (btn == BUTTON_A) {
            Serial.println("A pressed");
        }
        
        // Accelerometer
        AccelState accel = wiimote.getAccelState();
        Serial.printf("Accel: %d, %d, %d\n", 
            accel.xAxis, accel.yAxis, accel.zAxis);
        
        // Nunchuk
        NunchukState nunchuk = wiimote.getNunchukState();
        Serial.printf("Stick: %d, %d\n", 
            nunchuk.xStick, nunchuk.yStick);
    }
    
    // Battery check (every 60 seconds)
    static unsigned long lastBattery = 0;
    if (millis() - lastBattery > 60000) {
        uint8_t battery = wiimote.getBatteryLevel();
        Serial.printf("Battery: %d%%\n", battery);
        lastBattery = millis();
    }
}
```

---

## See Also

- [Testing Guide](TESTING.md) - Running unit tests
- [Logging System](LOGGING.md) - Debug output configuration
- [Architecture](ARCHITECTURE.md) - Internal system design
