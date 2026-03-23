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

#### `bool init()`

Initializes Bluetooth controller and HCI interface. Must be called once in `setup()`.

**Returns:** `true` on success, `false` if Bluetooth initialization fails

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

### Output Control

#### `bool setLeds(uint8_t ledMask)`

Sets the Wiimote LED bitmask.

**Parameters:**

- `ledMask` - LED bitmask (bit 0..3 map to LEDs 1..4)

**Returns:** `true` if command was queued, `false` if not connected

**Preconditions / guards:**

- Requires an active Wiimote connection
- Returns `false` when no connection is available

#### `bool setReportingMode(ReportingMode mode, bool continuous = false)`

Sets Wiimote input reporting mode.

**Parameters:**

- `mode` - reporting mode enum value
- `continuous` - `true` for continuous reports, `false` for change-based reports

**Returns:** `true` if command was queued, `false` if not connected

**Preconditions / guards:**

- Requires an active Wiimote connection
- Returns `false` when no connection is available

#### `bool setAccelerometerEnabled(bool enabled)`

Enables or disables accelerometer usage in runtime parsing.

**Parameters:**

- `enabled` - `true` to enable accelerometer handling, `false` to disable

**Returns:** Always `true`

**Preconditions / guards:**

- No connection required
- This controls local runtime behavior and affects subsequent report handling

#### `bool requestStatus()`

Requests a status report from the currently connected controller.

**Returns:** `true` if command was queued, `false` if not connected

**Preconditions / guards:**

- Requires an active Wiimote connection
- Returns `false` when no connection is available

#### `bool writeMemory(uint8_t addressSpace, uint32_t offset, const uint8_t *data, uint8_t len)`

Writes data to Wiimote EEPROM or control-register address space.

**Parameters:**

- `addressSpace` - `WiimoteAddressSpace` value cast to `uint8_t`
- `offset` - address offset inside selected space
- `data` - pointer to payload bytes
- `len` - payload length

**Returns:** `true` if command was queued, `false` if not connected

**Preconditions / guards:**

- Requires an active Wiimote connection
- Returns `false` when no connection is available

#### `bool readMemory(uint8_t addressSpace, uint32_t offset, uint16_t size)`

Reads data from Wiimote EEPROM or control-register address space.

**Parameters:**

- `addressSpace` - `WiimoteAddressSpace` value cast to `uint8_t`
- `offset` - address offset inside selected space
- `size` - number of bytes to read

**Returns:** `true` if command was queued, `false` if not connected

**Preconditions / guards:**

- Requires an active Wiimote connection
- Returns `false` when no connection is available

---

### Bluetooth Controller Runtime Control

Controller runtime methods are guarded by a deterministic transition model documented in
`docs/ARCHITECTURE.md` under "Controller Command State Machine".

#### `void setScanEnabled(bool enabled)`

Enables or disables Bluetooth scan mode at runtime.

**Parameters:**

- `enabled` - `true` to enable scan mode, `false` to disable

**Preconditions / guards:**

- Controller runtime must be initialized (`init()` + runtime startup complete)
- When disconnected, redundant transitions are rejected internally (no command sent)

**Lifecycle behavior:**

- `scanning` is updated immediately from submitted scan commands
- `scanning` is forced to `false` when the first active controller connection is established

#### `bool startDiscovery()`

Starts inquiry/discovery flow.

**Returns:** `true` when start request is accepted, `false` when rejected by guards

**Preconditions / guards:**

- Controller runtime must be initialized (`init()` + runtime startup complete)
- Discovery is rejected if scanning is already active
- Discovery is rejected while already connected

#### `bool stopDiscovery()`

Stops inquiry/discovery flow.

**Returns:** `true` when stop request is accepted, `false` when rejected by guards

**Preconditions / guards:**

- Controller runtime must be initialized (`init()` + runtime startup complete)
- Stop request is rejected if scanning is not active
- Stop request is rejected while connected

#### `bool disconnectActiveController(DisconnectReason reason)`

Requests disconnect of the active controller.

**Parameters:**

- `reason` - HCI disconnect reason enum value

**Returns:** `true` when disconnect command is sent, `false` if no active connection

**Preconditions / guards:**

- Controller runtime must be initialized (`init()` + runtime startup complete)
- Requires an active Wiimote connection and valid connection handle

#### `void setAutoReconnectEnabled(bool enabled)`

Enables or disables auto-reconnect policy.

**Parameters:**

- `enabled` - `true` to enable policy, `false` to disable

**Preconditions / guards:**

- Controller runtime must be initialized (`init()` + runtime startup complete)
- If runtime is not initialized, the request is ignored

#### `void clearReconnectCache()`

Clears cached controller identity used by fast reconnect.

**Preconditions / guards:**

- Controller runtime must be initialized (`init()` + runtime startup complete)
- If runtime is not initialized, the request is ignored

#### `BluetoothControllerState getBluetoothControllerState()`

Returns a snapshot of Bluetooth controller runtime state.

**Returns:** `BluetoothControllerState` with initialization, start, scan, connection, and reconnect flags

#### Controller Guard Semantics

Runtime controller operations are validated before HCI command submission. Invalid
transitions are rejected deterministically.

Scanning-state lifecycle is command-driven: accepted scan-enable commands set
`scanning=true`, accepted scan-disable commands set `scanning=false`, and
connection establishment forces `scanning=false`.

| From state                           | Allowed commands                                                                                                           |
| ------------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| Not started                          | none (all rejected)                                                                                                        |
| Started, not connected, not scanning | `setScanEnabled(true)`, `startDiscovery`, reconnect-policy/cache operations                                                |
| Started, scanning                    | `stopDiscovery`, `setScanEnabled(false)`, reconnect-policy/cache operations                                                |
| Started, connecting                  | none (intermediate state; transitions complete from HCI events)                                                            |
| Connected                            | `requestStatus`, `setLeds`, `setReportingMode`, `setAccelerometerEnabled`, `disconnect`, reconnect-policy/cache operations |
| Connected                            | `setScanEnabled(...)` is allowed but does not affect the active connection                                                 |

Deterministic rejection examples:

- `startDiscovery()` while already scanning/discovering -> rejected
- `stopDiscovery()` while discovery is idle -> rejected
- `disconnectActiveController(...)` while disconnected -> rejected
- Any controller operation before runtime initialization -> rejected/no-op (by method contract)

---

### Public Types

#### `enum class ReportingMode : uint8_t`

Supported reporting modes exposed by the public API:

- `CoreButtons` (`0x30`)
- `CoreButtonsAccel` (`0x31`)
- `CoreButtonsAccelIr` (`0x33`)
- `CoreButtonsAccelExt` (`0x35`)

#### `enum class DisconnectReason : uint8_t`

Supported disconnect reason codes:

- `LocalHostTerminated` (`0x16`)
- `RemoteUserTerminated` (`0x13`)
- `AuthenticationFailure` (`0x05`)
- `PowerOff` (`0x15`)

#### `struct BluetoothControllerState`

Runtime controller-state snapshot fields:

- `initialized`
- `started`
- `scanning`
- `connected`
- `activeConnectionHandle`
- `fastReconnectActive`
- `autoReconnectEnabled`

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
