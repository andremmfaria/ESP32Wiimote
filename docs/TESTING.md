# Quick Start: Testing ESP32Wiimote

## Prerequisites

Install PlatformIO:

```bash
pip install platformio
```

## Running Tests

### 0. Unified Build Script (Recommended)

Use the root build script so local, CI, and team workflows use the same commands:

```bash
./build.sh help
```

Common targets:

```bash
./build.sh test:native
./build.sh test:native:list
./build.sh test:coverage
./build.sh test:dev
./build.sh test:dev:build
./build.sh build:dev
./build.sh release
./build.sh clang:updatedb
./build.sh clang:tidy
```

Hardware targets accept `ESP32_PORT`:

```bash
ESP32_PORT=/dev/ttyUSB0 ./build.sh test:dev
ESP32_PORT=/dev/ttyUSB0 ./build.sh upload:dev
ESP32_PORT=/dev/ttyUSB0 ./build.sh monitor:dev
```

### 1. Native Tests (Recommended for Development)

Run all native tests on your PC (no hardware needed):

```bash
./build.sh test:native
```

Run specific test:

```bash
./build.sh test:native:button_state
./build.sh test:native:sensor_state
./build.sh test:native:wifi_router
./build.sh test:native:esp32wiimote
```

Verbose output:

```bash
./build.sh test:native -- -v
```

**Advantages:**

- ⚡ Fast (runs in ~1 second)
- 🔄 No upload time
- 💻 No hardware required
- 🐛 Easy debugging with gdb

### 2. Embedded Tests (Hardware Integration)

Upload and run tests on ESP32:

```bash
ESP32_PORT=/dev/ttyUSB0 ./build.sh test:dev
```

**Requirements:**

- ESP32 board connected via USB
- Wiimote for connection tests
- Serial monitor at 115200 baud

### 2.1 Non-Interactive Live Test (Hardware)

Run the dedicated non-interactive embedded Wi-Fi suite and capture serial output to a log file:

```bash
ESP32_PORT=/dev/ttyUSB0 ./scripts/run-live-noninteractive.sh
```

Optional parameters:

```bash
./scripts/run-live-noninteractive.sh embedded/test_wifi_noninteractive artifacts/live-tests
```

### 3. Build Only (No Tests)

Compile for ESP32 without running tests:

```bash
./build.sh build:dev
```

Compile embedded tests without upload/execution:

```bash
./build.sh test:dev:build
```

### 4. Coverage

Generate native line/branch coverage reports:

```bash
./build.sh test:coverage
```

Outputs:

- `coverage/gcovr-summary.txt`
- `coverage/gcovr-covered-summary.txt`
- `coverage/gcovr.csv`
- `coverage/gcovr.xml`
- `coverage/html-gcovr/index.html`
- `coverage/src-coverage-status.txt`
- `coverage/lcov.info`
- `coverage/html-lcov/index.html`

### 5. Clang Tooling

Generate or refresh the compile database used by clang-based tooling:

```bash
./build.sh clang:updatedb
```

Run `clang-tidy` across the default set of production files:

```bash
./build.sh clang:tidy
```

Run `clang-tidy` against specific files only:

```bash
./build.sh clang:tidy src/ESP32Wiimote.cpp src/wifi/web_api_router.cpp
```

## Test Structure

```text
test/
├── native/                      # Fast unit/component tests (PC)
│   ├── test_button_state/
│   ├── test_data_parser/
│   ├── test_esp32wiimote/
│   ├── test_hci_codes/
│   ├── test_hci_stack/
│   ├── test_protocol/
│   ├── test_sensor_state/
│   ├── test_serial/
│   ├── test_serial_dispatcher/
│   ├── test_serial_formatter/
│   ├── test_serial_session/
│   ├── test_stack_components/
│   ├── test_tinywiimote_core/
│   ├── test_utils/
│   └── test_wiimote_extensions/
├── embedded/                    # Hardware tests (ESP32)
│   ├── test_bluetooth/
│   └── test_integration/
└── mocks/               # Test infrastructure (native tests only)
    ├── test_mocks.h     # Mock state and inline validation callback
    ├── test_mocks.cpp   # TinyWiimote stubs, L2CAP packet framing
    └── Arduino.h        # Arduino API stub for native builds
```

### Native Suite Coverage Map

- `test_button_state/`, `test_sensor_state/`: button/sensor state transition logic
- `test_data_parser/`: Wiimote report parsing and filter behavior
- `test_protocol/`, `test_wiimote_extensions/`: protocol and extension handling
- `test_hci_codes/`, `test_hci_stack/`, `test_stack_components/`: HCI/lower-stack command, event, and packet flows
- `test_tinywiimote_core/`, `test_utils/`: TinyWiimote core and utility helpers
- `test_esp32wiimote/`: public API delegation and task-loop integration behavior
- `test_serial*`: serial parser/dispatcher/formatter/session and end-to-end serial task-loop behavior

### Wi-Fi HTTP Static Asset Coverage

The embedded browser assets are validated in two places:

- `test:native:wifi_router` verifies unauthenticated static route handling for `/`, `/app.js`, `/styles.css`, and `/openapi.json`
- `test:native:esp32wiimote` verifies the Wi-Fi HTTP bridge can serve the embedded assets through the mock server path

Implementation details that matter for testing:

- the C++ translation units in `src/wifi/web/*.cpp` are the sole source of truth for asset content; there are no separate `.html`, `.js`, or `.css` source files
- the ESP32 HTTP response buffer was increased to 8192 bytes so the browser asset payloads fit without truncation

### Test Mock Infrastructure

Native tests use a minimal mock layer in `test/mocks/` to isolate hardware dependencies:

**`test_mocks.h/cpp`:**

- **TinyWiimote boundary mocks** - `TinyWiimoteAvailable()`, `TinyWiimoteRead()` allow injecting test HID reports
- **L2CAP packet capture** - `mockL2capRawSendCallback()` validates ACL/L2CAP framing and captures payloads
- **Packet framing implementation** - `make_acl_l2cap_packet()`, `make_l2cap_packet()` provide real packet construction
- **Mock state variables** - Track last packet, call counts, connection handles for assertions

**`Arduino.h`:**

- Minimal Arduino API stub (Serial, delay, millis) for native compilation
- Includes controllable mock serial input/output buffers for serial task-loop tests
- Includes controllable mock `millis()` clock for unlock-window expiry tests

Additional mock headers under `test/mocks/` provide compile-time stubs for ESP32/BT platform headers used by production code.

**Key Design:**

- ✅ **Production code is clean** - no `#ifdef NATIVE_TEST` or test stubs in `src/`
- ✅ **Validates real behavior** - tests exercise actual parser/protocol/connection implementations
- ✅ **Automatic packet validation** - every L2CAP packet checked for correct ACL/L2CAP headers
- ✅ **Boundary-only mocking** - only hardware input/output boundaries are mocked

### Serial-focused native suites

- `test_serial/` validates parser limits and tokenization behavior
- `test_serial_dispatcher/` validates command mapping, argument parsing, and lock gating
- `test_serial_formatter/` validates stable response contracts and bounded writes
- `test_serial_session/` validates unlock-window timing and expiry semantics
- `test_esp32wiimote/` validates task-loop serial integration (one line per `task()` call)

## Writing Tests

### Native Test Pattern (`test/native/...`)

Create `test/native/test_myfeature/test_myfeature.cpp`:

```cpp
#include <unity.h>
#include "YourClass.h"

YourClass* instance;

void setUp(void) {
    instance = new YourClass();
}

void tearDown(void) {
    delete instance;
}

void test_something(void) {
    TEST_ASSERT_EQUAL(42, instance->getSomething());
}

int main(int /*argc*/, char ** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_something);
    return UNITY_END();
}
```

Run it:

```bash
./build.sh test:native -- -f test_myfeature
```

### Embedded Test Pattern (`test/embedded/...`)

Embedded tests run with Arduino lifecycle entrypoints and usually require serial output for prompts:

```cpp
void setup() {
    Serial.begin(115200);
    UNITY_BEGIN();
    RUN_TEST(test_hardware_scenario);
    UNITY_END();
}

void loop() {
    // Optional background processing
}
```

Run embedded tests:

```bash
ESP32_PORT=/dev/ttyUSB0 ./build.sh test:dev -- -f test_integration -v
```

## Continuous Integration

Tests run automatically on every push via GitHub Actions:

- ✅ All native tests
- ✅ ESP32 compilation check

See [.github/workflows/platformio-ci.yml](.github/workflows/platformio-ci.yml)

## Debugging Tests

### Native Tests with GDB

```bash
cd .pio/build/native
gdb program
```

### ESP32 Tests

View output in serial monitor:

```bash
pio device monitor -e esp32dev -b 115200
```

## Common Unity Assertions

```cpp
TEST_ASSERT_TRUE(condition);
TEST_ASSERT_FALSE(condition);
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_EQUAL_UINT8(expected, actual);
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);
TEST_PASS_MESSAGE("success");
TEST_FAIL_MESSAGE("failure reason");
TEST_IGNORE_MESSAGE("skipped because...");
```

## Next Steps

1. ✅ Run existing tests: `pio test -e native`
2. 📝 Add tests for your feature
3. 🔄 Run tests frequently during development
4. 🚀 Push changes (CI runs automatically)

## Troubleshooting

### "No tests were run"

- Check suite name matches the folder under `test/native/` or `test/embedded/`
- For native: `./build.sh test:native -- -f test_serial_dispatcher`
- For embedded: `ESP32_PORT=/dev/ttyUSB0 ./build.sh test:dev -- -f test_integration -v`

### "command not found: pio"

```bash
pip install --upgrade platformio
export PATH=$PATH:~/.local/bin
```

### "No such file or directory: ESP32Wiimote.h"

- Set `test_build_src = yes` in platformio.ini
- Already configured in this project ✓

### Native tests fail with missing Arduino.h

- Ensure `test/mocks/Arduino.h` exists (provides Arduino API stub)
- Check `platformio.ini` includes `-I test/mocks` in build_flags
- Already configured in this project ✓

### Tests pass but changes to production code don't fail tests

- Verify `platformio.ini` build_src_filter includes real source files (not stubs)
- Check tests use real implementations from `src/`, not full mocks
- Current setup: tests exercise production code ✓

## Resources

- [PlatformIO Testing Docs](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [Unity Assertions](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
- [Example Tests](test/native/)
