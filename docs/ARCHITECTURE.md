# System Architecture

ESP32Wiimote is designed with a layered architecture separating hardware interface, protocol handling, and application API.

## Table of Contents

- [Architecture Overview](#architecture-overview)
- [Layer Details](#layer-details)
- [Component Responsibilities](#component-responsibilities)
- [Data Flow](#data-flow)
- [Threading Model](#threading-model)
- [Controller Command State Machine](#controller-command-state-machine)
- [Serial Command Pipeline](#serial-command-pipeline)
- [Wi-Fi API Pipeline](#wi-fi-api-pipeline)

---

## Architecture Overview

```text
┌─────────────────────────────────────────────────────────┐
│                     Application Layer                   │
│              (ESP32Wiimote public API)                  │
└─────────────────────────────────────────────────────────┘
                          │
                          ↓
┌─────────────────────────────────────────────────────────┐
│                    State Management                     │
│    ButtonStateManager │ SensorStateManager              │
│    WiimoteDataParser                                    │
└─────────────────────────────────────────────────────────┘
                          │
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Protocol Layer                        │
│    WiimoteProtocol │ WiimoteExtensions                  │
│    WiimoteState │ WiimoteReports                        │
└─────────────────────────────────────────────────────────┘
                          │
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   L2CAP Layer                           │
│    L2capSignaling │ L2capConnection                     │
│    L2capPacketSender                                    │
└─────────────────────────────────────────────────────────┘
                          │
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   HCI Layer                             │
│    HciEventContext │ HciCommands                        │
│    HciQueueManager (TX/RX queues)                       │
└─────────────────────────────────────────────────────────┘
                          │
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Hardware Layer                        │
│    ESP32 Bluetooth Controller (VHCI)                    │
└─────────────────────────────────────────────────────────┘
```

---

## Layer Details

### Application Layer

**Location:** `src/ESP32Wiimote.{h,cpp}`

**Purpose:** User-facing API with high-level methods

**Key Classes:**

- `ESP32Wiimote` - Main interface class

**Responsibilities:**

- Initialize all subsystems
- Provide simple API (buttons, sensors, connection status)
- Manage component lifecycle
- Abstract implementation details

**Usage:**

```cpp
ESP32Wiimote wiimote;
wiimote.init();
wiimote.task();
if (wiimote.available()) {
    ButtonState btn = wiimote.getButtonState();
}
```

---

### State Management Layer

**Location:** `src/esp32wiimote/state/`, `src/esp32wiimote/data_parser.cpp`

**Purpose:** Track and parse Wiimote sensor/button data

**Key Classes:**

- `ButtonStateManager` - Button state tracking with change detection
- `SensorStateManager` - Accelerometer and nunchuk state tracking
- `WiimoteDataParser` - Parse HID reports into button/sensor states

**Responsibilities:**

- Decode HID input reports (0x30-0x37)
- Track current vs previous state
- Detect state changes
- Apply threshold logic for analog sensors
- Implement filtering

**Data Structures:**

```cpp
struct AccelState {
    uint8_t xAxis, yAxis, zAxis;
};

struct NunchukState {
    uint8_t xStick, yStick;
    uint8_t xAccel, yAccel, zAccel;
    bool cBtn, zBtn;
};
```

---

---

### Test Infrastructure Layer

**Location:** `test/mocks/`

**Purpose:** Provide minimal mocks for native testing without modifying production code

**Key Files:**

- `test_mocks.h/cpp` - Mock implementations and state
- `Arduino.h` - Arduino framework stub

**Responsibilities:**

- Simulate TinyWiimote hardware input (inject test HID reports)
- Capture and validate L2CAP packet output
- Implement packet framing for test environment
- Provide Arduino API stubs for native compilation
- Track mock state for test assertions

**Mock Boundaries:**

```cpp
// Input boundary - inject test data
int TinyWiimoteAvailable(void);  // Returns mockHasData
TinyWiimoteData TinyWiimoteRead(void);  // Returns mockData

// Output boundary - capture and validate
void mockL2capRawSendCallback(uint8_t* data, size_t len);
// Validates: H4 type, ACL handle/length, L2CAP CID/length
// Captures: Wiimote HID payload in mockLastPacket[]
```

**Design Principles:**

- ✅ Production code remains clean (no test conditionals)
- ✅ Tests exercise real implementations
- ✅ Automatic validation of packet framing
- ✅ Minimal mock surface area (boundaries only)

---

### Protocol Layer (TinyWiimote)

**Location:** `src/tinywiimote/protocol/`

**Purpose:** Wiimote-specific protocol implementation

**Key Classes:**

- `WiimoteProtocol` - Output reports (LEDs, reporting mode, memory R/W, status request)
- `WiimoteExtensions` - Extension detection and Nunchuk handling
- `WiimoteState` - Connection state and battery level
- `WiimoteReports` - Input report buffering/queue

**Responsibilities:**

- Build Wiimote output reports (0xA2 prefix)
- Parse input reports (0xA1 prefix)
- Handle extension controller detection
- Manage battery status reports (0x20)
- Queue incoming HID data

**Output Reports:**

```text
0x11 - Set LEDs
0x12 - Set Reporting Mode
0x15 - Request Status
0x16 - Write Memory
0x17 - Read Memory
```

**Input Reports:**

```text
0x20 - Status Information (battery, extensions)
0x21 - Read Memory Response
0x30 - Core Buttons
0x31 - Core Buttons + Accelerometer
0x32 - Core Buttons + 8 Extension Bytes
0x35 - Core Buttons + Accelerometer + 16 Extension Bytes
```

---

### L2CAP Layer

**Location:** `src/tinywiimote/l2cap/`

**Purpose:** Bluetooth L2CAP protocol handling

**Key Classes:**

- `L2capSignaling` - Connection request/response, configuration
- `L2capConnection` - Connection state tracking
- `L2capConnectionTable` - Connection bookkeeping helpers
- `L2capPacketSender` - ACL packet construction

**Responsibilities:**

- L2CAP channel establishment
- MTU negotiation
- Channel ID tracking
- Encapsulate HID data in L2CAP frames
- Enforce a single active Wiimote session in the production path

**Connection Sequence:**

```text
1. CONNECTION REQUEST  (0x02) → PSM 0x0013 (HID Control)
2. CONNECTION RESPONSE (0x03) ← Remote CID assigned
3. CONFIGURATION REQ   (0x04) → MTU, flags
4. CONFIGURATION RESP  (0x05) ← Success
```

---

### HCI Layer

**Location:** `src/tinywiimote/hci/`, `src/esp32wiimote/`

**Purpose:** Low-level Bluetooth HCI operations

**Key Classes:**

- `HciEventContext` - Event handling state machine
- `HciCommands` - Build HCI command packets
- `HciQueueManager` - TX/RX packet queuing (FreeRTOS queues)
- `HciCallbacksHandler` - VHCI callback wrappers

**Responsibilities:**

- HCI command/event handling
- Device inquiry and discovery
- ACL connection management
- Packet queueing for async operation
- VHCI interface integration

**Connection Flow:**

```text
1. RESET (0x03 0x0C)
2. READ_BD_ADDR (0x09 0x10)
3. WRITE_SCAN_ENABLE (0x1A 0x0C)
4. INQUIRY (0x01 0x04) - Search for Wiimote
5. INQUIRY_RESULT - Device found
6. REMOTE_NAME_REQUEST (0x19 0x04) - "Nintendo RVL-CNT-01"
7. CREATE_CONNECTION (0x05 0x04)
8. CONNECTION_COMPLETE
```

---

## Controller Command State Machine

Controller runtime commands use deterministic guards so transitions are explicitly
accepted or rejected before command submission.

| From state                           | Allowed commands                                                                                                           |
| ------------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| Not started                          | none (all rejected)                                                                                                        |
| Started, not connected, not scanning | `setScanEnabled(true)`, `startDiscovery`, reconnect-policy/cache operations                                                |
| Started, scanning                    | `stopDiscovery`, `setScanEnabled(false)`, reconnect-policy/cache operations                                                |
| Started, connecting                  | none (intermediate state; transitions complete from HCI events)                                                            |
| Connected                            | `requestStatus`, `setLeds`, `setReportingMode`, `setAccelerometerEnabled`, `disconnect`, reconnect-policy/cache operations |
| Connected                            | `setScanEnabled(...)` is allowed but does not affect the active connection                                                 |

Guard behavior requirements:

- Invalid transitions are rejected deterministically.
- Rejected transitions are logged at warning level with the rejection reason.
- Guards are evaluated before HCI command submission.
- `scanningEnabled` is set from accepted scan/discovery command submissions and
  is forced to `false` when a controller connection is established.

Recommended deterministic rejection mapping:

- `startDiscovery()` while scanning/discovering -> reject (`false`)
- `stopDiscovery()` while idle -> reject (`false`)
- `disconnectActiveController()` while not connected -> reject (`false`)
- `setScanEnabled(false)` while not started -> reject internally (no-op)
- reconnect-policy/cache commands while not started -> reject internally (no-op)

---

## Serial Command Pipeline

Serial runtime control is implemented as a bounded pipeline that executes in
`ESP32Wiimote::task()`.

Components:

- Parser: `src/serial/serial_command_parser.{h,cpp}`
- Dispatcher: `src/serial/serial_command_dispatcher.{h,cpp}`
- Response formatter: `src/serial/serial_response_formatter.{h,cpp}`
- Unlock session: `src/serial/serial_command_session.{h,cpp}`

Execution flow (one line per task call):

1. Read serial bytes into a bounded line buffer
2. On newline, parse tokens (`wm` prefix)
3. Dispatch command to public API target
4. Format deterministic response text and emit via serial

Bounds and behavior:

- Input line length bounded to 128 bytes
- Token count bounded to 10
- At most one completed command line is processed per `task()` call
- Non-command lines are ignored
- Line overflow returns `@wm: error line_too_long`

Privileged-command gating:

- Write/control serial commands are locked by default
- `wm unlock <token> [seconds]` opens a token-validated unlock window
- Once expired, privileged commands are rejected with `locked`

Integration note:

- Serial command handlers call the same public runtime methods used by firmware code, preserving a single behavior surface.

---

## Wi-Fi API Pipeline

Wi-Fi runtime control is exposed through a static-plus-REST router in `src/wifi/web_api_router.cpp`.

Components:

- Auth validator: `src/wifi/web_auth.{h,cpp}`
- Request parser: `src/wifi/web_request_parser.{h,cpp}`
- Response serializer: `src/wifi/web_response_serializer.{h,cpp}`
- Router and static assets: `src/wifi/web_api_router.{h,cpp}`, `src/wifi/web/`
- Async command queue + command status readback: `src/wifi/web_command_queue.{h,cpp}`
- Split event stream buffering/replay: `src/wifi/web_event_stream.{h,cpp}`

Request flow:

1. Static route short-circuit (`/`, `/app.js`, `/styles.css`, `/openapi.json`)
2. Auth enforcement (Bearer token)
3. Route lookup by method + path
4. Body parse for `POST` routes
5. Public API callback dispatch
6. Deterministic status mapping and JSON serialization

HTTP mapping:

- `200`: read success or command accepted
- `400`: malformed body, missing required fields, invalid argument
- `401`: missing/invalid token
- `403`: reserved for future policy-based restrictions
- `404`: unknown route
- `409`: runtime/state guard rejected command

Lifecycle integration:

- Wi-Fi startup is staged asynchronously in `ESP32Wiimote::task()`
- startup order is Wi-Fi layer -> filesystem mount -> static routes -> API routes
- if delivery mode is `RestAndWebSocket`, websocket routes are registered before ready
- `RestOnly` mode enables REST routes only; `RestAndWebSocket` includes websocket route stage

Event stream model:

- channels: `/api/wiimote/input/events` and `/api/wiimote/status/events`
- each channel has bounded buffering and independent monotonic `seq`
- replay is sequence-driven; if a gap is detected the client must recover from REST snapshots

Runtime configuration integration:

- runtime serial/wifi tokens and network credentials are provided through `WiimoteConfig`
- Wi-Fi station join is attempted before API route readiness
- reconnect policy fields are persisted in NVS via `RuntimeConfigStore` and restored on startup

---

### Hardware Layer

**Location:** ESP32 Bluetooth Controller

**Purpose:** Hardware Bluetooth radio

**Key Components:**

- ESP32 Bluetooth Classic controller
- VHCI (Virtual HCI) interface

**Responsibilities:**

- Radio transmission/reception
- Baseband processing
- Hardware-accelerated Bluetooth stack
- This project targets one active Wii Remote per ESP32 radio due to Bluetooth Classic HCI limits

**Interface:**

```cpp
esp_vhci_host_register_callback(&callbacks);
esp_vhci_host_send_packet(data, len);
esp_vhci_host_check_send_available();
```

---

## Component Responsibilities

### ESP32Wiimote

- Initialize subsystems
- Coordinate component interaction
- Provide user API
- Manage filters

### BluetoothController

- ESP32 Bluetooth initialization
- Start/stop BT controller
- VHCI registration

### HciQueueManager

- TX/RX FreeRTOS queues
- Queue data structures
- Packet buffering

### HciCallbacksHandler

- VHCI callback registration
- Route packets to queues
- Bridge VHCI ↔ TinyWiimote

### TinyWiimote (Core)

- Low-level protocol state machine
- Coordinate all protocol layers
- Route packets between layers

### ButtonStateManager

- Button state tracking
- Change detection
- Previous/current state

### SensorStateManager

- Accelerometer tracking
- Nunchuk stick tracking
- Threshold-based change detection

### WiimoteDataParser

- Parse HID input reports
- Update state managers
- Apply filters

---

## Data Flow

### Outgoing (TX - ESP32 → Wiimote)

```text
Application
  ↓ wiimote.requestBatteryUpdate()
ESP32Wiimote
  ↓ TinyWiimoteRequestBatteryUpdate()
TinyWiimote Runtime (g_runtime)
  ↓ g_runtime.wiimoteProtocol.requestStatus(ch)
WiimoteProtocol
  ↓ sender->sendAclL2capPacket(ch, remoteCID, payload, len)
L2capPacketSender
  ↓ make_acl_l2cap_packet() → sendCallback()
HCI Send Callback
  ↓ HciQueueManager->sendToTxQueue(data, len)
HciQueueManager TX Queue
  ↓ (processed by task loop)
processTxQueue()
  ↓ esp_vhci_host_send_packet(data, len)
ESP32 Bluetooth Controller
  ↓ (radio transmission)
Wiimote
```

### Incoming (RX - Wiimote → ESP32)

```text
Wiimote
  ↓ (radio reception)
ESP32 Bluetooth Controller
  ↓ VHCI callback: notify_host_recv(data, len)
HciCallbacksHandler::notifyHostRecv()
  ↓ queueManager->sendToRxQueue(data, len)
HciQueueManager RX Queue
  ↓ (processed by task loop)
processRxQueue()
  ↓ handleHciData(data, len)
TinyWiimote Event Dispatcher
  ↓ parse packet type (HCI Event vs ACL Data)
HciEvents or L2CAP Handler
  ↓ route to appropriate handler
WiimoteExtensions::handleReport()
  ↓ parse input report, update WiimoteState
WiimoteReports Queue
  ↓ buffer HID data
WiimoteDataParser::parseData()
  ↓ TinyWiimoteRead() → parse buttons/sensors
ButtonStateManager & SensorStateManager
  ↓ update current state, set change flags
Application
  ↓ wiimote.available() returns true
  ↓ wiimote.getButtonState() / getAccelState()
```

---

## Threading Model

### Main Thread

All operations run in the main Arduino loop:

```cpp
void loop() {
    wiimote.task();           // Process queues
    if (wiimote.available()) { // Parse data
        // Handle input
    }
}
```

### Queue Processing

- **TX Queue:** Drained by `processTxQueue()` when VHCI ready
- **RX Queue:** Filled by VHCI callback, drained by `processRxQueue()`

### FreeRTOS Integration

- Uses FreeRTOS queues (`xQueueCreate`, `xQueueSend`, `xQueueReceive`)
- No separate tasks created
- VHCI callbacks may run in BT controller task context
- Queues provide thread-safe handoff

### Critical Sections

Minimal locking needed:

- VHCI callbacks → RX queue (FreeRTOS queue handles sync)
- Main loop → TX queue (single-threaded producer)

---

## Memory Management

### Static Allocation

Most structures use stack or static allocation:

- HCI/L2CAP packet buffers
- State structures
- Connection tables

### Dynamic Allocation

Limited dynamic allocation:

- `HciQueueData` allocated per packet (freed after processing)
- Component managers created once in ESP32Wiimote constructor

### Queue Memory

FreeRTOS queues store pointers to `HciQueueData`:

```cpp
struct HciQueueData {
    size_t len;
    uint8_t data[]; // Flexible array
};
```

---

## Configuration Points

### Logging

Defined by the application before including the library, or via build flags:

```cpp
#define WIIMOTE_VERBOSE 2  // 0-3
```

### Queue Sizes

`ESP32Wiimote.cpp`:

```cpp
_queueManager = new HciQueueManager(32, 32); // TX, RX size
```

### Nunchuk Threshold

`ESP32WiimoteConfig`:

```cpp
ESP32WiimoteConfig config;
config.nunchukStickThreshold = 5;  // Stick sensitivity
ESP32Wiimote wiimote(config);
```

### Runtime Auth and Wi-Fi Policy

Runtime tokens and Wi-Fi enablement are configured through `WiimoteConfig`:

```cpp
WiimoteConfig runtimeConfig = {
  true,
  "esp32wiimote_serial_token_v1",
  "esp32wiimote_wifi_api_token_v1"
};

ESP32Wiimote wiimote;
wiimote.configure(runtimeConfig);
```

### Wi-Fi Lifecycle Control

Wi-Fi startup is controlled independently and progresses asynchronously in `task()`:

```cpp
wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
bool ready = wiimote.isWifiControlReady();
ESP32Wiimote::WifiControlState state = wiimote.getWifiControlState();
```

---

## Extensibility

### Adding New Input Reports

1. Add report ID constant in `wiimote_protocol.cpp`
2. Handle in `WiimoteDataParser::parseData()`
3. Extract data fields
4. Update state managers

### Adding New Output Reports

1. Add opcode constant in `wiimote_protocol.cpp`
2. Add method to `WiimoteProtocol`
3. Build packet with `PayloadBuilder`
4. Send via `L2capPacketSender`

### Adding New Sensors

1. Add state struct in `sensor_state.h`
2. Add getter/setter in `SensorStateManager`
3. Parse data in `WiimoteDataParser`
4. Expose via `ESP32Wiimote` API

---

## Design Principles

### Separation of Concerns

Each layer handles one responsibility:

- Hardware: Radio
- HCI: Commands/events
- L2CAP: Channels
- Protocol: Wiimote specifics
- State: Data tracking
- API: User interface

### Dependency Inversion

High-level code depends on abstractions:

- `WiimoteProtocol` uses `L2capPacketSender` interface
- `L2capSignaling` uses connection table abstraction

### Single Responsibility

Each class has one job:

- `ButtonStateManager`: Only buttons
- `WiimoteProtocol`: Only Wiimote output reports
- `HciQueueManager`: Only queue operations

### Testability

Components can be tested independently:

- State managers tested without hardware
- Protocol layer tested with mocks
- Native tests run on PC

---

## See Also

- [API Reference](API.md) - Public API documentation
- [Testing Guide](TESTING.md) - Running tests
- [Logging System](LOGGING.md) - Debug output
