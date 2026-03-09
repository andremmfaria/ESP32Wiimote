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
pio test -e esp32dev --upload-port /dev/ttyUSB0
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
│   └── test_data_parser/
└── embedded/            # Hardware integration tests (ESP32)
    └── test_integration/
```

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
- 📊 Test reports uploaded as artifacts

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

- Add `#ifndef NATIVE_TEST` guards around Arduino-specific code
- Use mocks for hardware dependencies

## Resources

- [PlatformIO Testing Docs](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [Unity Assertions](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
- [Example Tests](test/native/)
