# ESP32Wiimote Testing Guide

## Test Structure

```text
test/
├── native/                    # Tests that run on PC (no hardware)
│   ├── test_button_state/
│   ├── test_sensor_state/
│   ├── test_data_parser/
│   └── test_protocol/
├── embedded/                  # Tests that run on ESP32 hardware
│   ├── test_bluetooth/
│   └── test_integration/
└── README.md                  # This file
```

## Running Tests

### Native Tests (Fast, No Hardware Required)

```bash
# Run all native tests
pio test -e native

# Run specific test
pio test -e native -f test_button_state

# Verbose output
pio test -e native -v
```

### Embedded Tests (Requires ESP32)

```bash
# Upload and run on ESP32 (basic output)
pio test -e esp32dev --upload-port /dev/ttyUSB0

# Run specific test with verbose output (REQUIRED for interactive tests)
# The -v flag is necessary to see user instructions like "Press 1+2 on Wiimote"
pio test -e esp32dev --upload-port /dev/ttyUSB0 -f test_integration -v

# Without -v, you'll only see test results (PASS/FAIL), not interaction prompts
pio test -e esp32dev --upload-port /dev/ttyUSB0 -f test_integration
```

**Note:** Integration tests require user interaction (pressing Wiimote buttons). Always use the `-v` flag to see the action prompts.

## Writing Tests

### Native Tests

- Test pure logic and algorithms
- Fast iteration (no upload time)
- Use mocks for hardware dependencies
- Focus on edge cases and data validation

### Embedded Tests

- Test actual hardware interaction
- Verify Bluetooth communication
- End-to-end integration tests
- Connection and timing tests

## Test Coverage Goals

- [ ] ButtonStateManager: 100% (pure logic)
- [ ] SensorStateManager: 100% (pure logic)
- [ ] WiimoteDataParser: 90%+ (data parsing)
- [ ] Protocol handlers: 80%+ (protocol logic)
- [ ] Integration tests: Key scenarios
- [ ] Hardware tests: Connection & communication

## CI/CD Integration

Native tests can run in GitHub Actions or any CI system:

```yaml
- name: Run Tests
  run: |
    pip install platformio
    pio test -e native
```
