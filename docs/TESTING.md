# Quick Start: Testing ESP32Wiimote

## Prerequisites

Install PlatformIO:

```bash
pip install platformio
```

## Running Tests

### 1. Native Tests (Recommended for Development)

Run all native tests on your PC (no hardware needed):

```bash
cd /path/to/ESP32Wiimote
pio test -e native
```

Run specific test:

```bash
pio test -e native -f test_button_state
pio test -e native -f test_sensor_state
```

Verbose output:

```bash
pio test -e native -v
```

**Advantages:**

- ⚡ Fast (runs in ~1 second)
- 🔄 No upload time
- 💻 No hardware required
- 🐛 Easy debugging with gdb

### 2. Embedded Tests (Hardware Integration)

Upload and run tests on ESP32:

```bash
pio test -e esp32dev --upload-port /dev/ttyUSB0 -v
```

**Requirements:**

- ESP32 board connected via USB
- Wiimote for connection tests
- Serial monitor at 115200 baud

### 3. Build Only (No Tests)

Compile for ESP32 without running tests:

```bash
pio run -e esp32dev
```

## Test Structure

```
test/
├── native/              # Fast unit tests (PC)
│   ├── test_button_state/
│   ├── test_sensor_state/
│   ├── test_data_parser/
│   └── test_protocol/
├── embedded/            # Hardware integration tests (ESP32)
│   └── test_integration/
└── mocks/               # Test infrastructure (native tests only)
    ├── test_mocks.h     # Mock state and inline validation callback
    ├── test_mocks.cpp   # TinyWiimote stubs, L2CAP packet framing
    └── Arduino.h        # Arduino API stub for native builds
```

### Test Mock Infrastructure

Native tests use a minimal mock layer in `test/mocks/` to isolate hardware dependencies:

**`test_mocks.h/cpp`:**

- **TinyWiimote boundary mocks** - `TinyWiimoteAvailable()`, `TinyWiimoteRead()` allow injecting test HID reports
- **L2CAP packet capture** - `mockL2capRawSendCallback()` validates ACL/L2CAP framing and captures payloads
- **Packet framing implementation** - `make_acl_l2cap_packet()`, `make_l2cap_packet()` provide real packet construction
- **Mock state variables** - Track last packet, call counts, connection handles for assertions

**`Arduino.h`:**

- Minimal Arduino API stub (Serial, delay, millis) for native compilation
- Required by `wiimote_protocol.cpp` include (unused but needed for compilation)

**Key Design:**

- ✅ **Production code is clean** - no `#ifdef NATIVE_TEST` or test stubs in `src/`
- ✅ **Validates real behavior** - tests exercise actual parser/protocol/connection implementations
- ✅ **Automatic packet validation** - every L2CAP packet checked for correct ACL/L2CAP headers
- ✅ **Boundary-only mocking** - only hardware input/output boundaries are mocked

## Writing Your First Test

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

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_something);
    UNITY_END();
}

void loop() {}
```

Run it:

```bash
pio test -e native -f test_myfeature
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

**"command not found: pio"**

```bash
pip install --upgrade platformio
export PATH=$PATH:~/.local/bin
```

**"No such file or directory: ESP32Wiimote.h"**

- Set `test_build_src = yes` in platformio.ini
- Already configured in this project ✓

**Native tests fail with missing Arduino.h**

- Ensure `test/mocks/Arduino.h` exists (provides Arduino API stub)
- Check `platformio.ini` includes `-I test/mocks` in build_flags
- Already configured in this project ✓

**Tests pass but changes to production code don't fail tests**

- Verify `platformio.ini` build_src_filter includes real source files (not stubs)
- Check tests use real implementations from `src/`, not full mocks
- Current setup: tests exercise production code ✓

## Resources

- [PlatformIO Testing Docs](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [Unity Assertions](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
- [Example Tests](test/native/)
