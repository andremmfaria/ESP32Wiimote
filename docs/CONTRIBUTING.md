# Contributing to ESP32Wiimote

Thank you for considering contributing to ESP32Wiimote! This document provides guidelines and instructions for contributors.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Code Organization](#code-organization)
- [Making Changes](#making-changes)
- [Testing](#testing)
- [Pull Request Process](#pull-request-process)
- [Coding Standards](#coding-standards)

---

## Getting Started

### Prerequisites

- **Git** for version control
- **PlatformIO** for building and testing
- **ESP32 board** for hardware testing
- **Wiimote** (Nintendo RVL-CNT-01) for integration tests

Install PlatformIO:
```bash
pip install platformio
```

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/ESP32Wiimote.git
   cd ESP32Wiimote
   ```

3. Add upstream remote:
   ```bash
   git remote add upstream https://github.com/ORIGINAL_OWNER/ESP32Wiimote.git
   ```

---

## Development Setup

### Install Dependencies

```bash
cd ESP32Wiimote
pio lib install
```

### Verify Setup

Run native tests:
```bash
pio test -e native
```

Expected output: `28 test cases: 28 succeeded`

---

## Code Organization

```
ESP32Wiimote/
├── src/
│   ├── ESP32Wiimote.{h,cpp}       # Public API
│   ├── TinyWiimote.{h,cpp}        # Core protocol
│   ├── esp32wiimote/              # ESP32-specific
│   │   ├── bt_controller.*        # BT initialization
│   │   ├── hci_callbacks.*        # VHCI callbacks
│   │   ├── queue/                 # HCI queues
│   │   └── state/                 # State managers
│   ├── tinywiimote/               # Protocol layers
│   │   ├── hci/                   # HCI commands/events
│   │   ├── l2cap/                 # L2CAP layer
│   │   └── protocol/              # Wiimote protocol
│   └── utils/
│       └── serial_logging.h       # Logging system
├── test/
│   ├── native/                    # PC unit tests
│   └── embedded/                  # ESP32 integration tests
├── docs/                          # Documentation
└── examples/                      # Arduino examples
```

See [Architecture](docs/ARCHITECTURE.md) for detailed design.

---

## Making Changes

### Create a Branch

```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/issue-description
```

Branch naming conventions:
- `feature/` - New features
- `fix/` - Bug fixes
- `docs/` - Documentation changes
- `test/` - Test additions/fixes
- `refactor/` - Code refactoring

### Make Your Changes

1. **Write code** following [coding standards](#coding-standards)
2. **Add tests** for new functionality
3. **Update documentation** if API changes
4. **Add logging** at appropriate levels

### Example: Adding a New API Method

```cpp
// 1. Add to ESP32Wiimote.h
class ESP32Wiimote {
public:
    void myNewMethod();
};

// 2. Implement in ESP32Wiimote.cpp
void ESP32Wiimote::myNewMethod() {
    LOG_DEBUG("myNewMethod called\n");
    // Implementation
}

// 3. Add test in test/native/test_myfeature/
void test_my_new_method() {
    ESP32Wiimote wiimote;
    wiimote.myNewMethod();
    TEST_ASSERT_EQUAL(expected, actual);
}

// 4. Document in docs/API.md
#### `void myNewMethod()`
Description of what it does...
```

---

## Testing

### Run All Tests

```bash
# Native tests (fast, no hardware)
pio test -e native

# Integration tests (requires ESP32 + Wiimote)
pio test -e esp32dev --upload-port /dev/ttyUSB0 -v
```

### Write Tests

**Unit Test Template:**

```cpp
// test/native/test_myfeature/test_myfeature.cpp
#include <unity.h>
#include "YourClass.h"

YourClass* instance;

void setUp(void) {
    instance = new YourClass();
}

void tearDown(void) {
    delete instance;
}

void test_basic_functionality(void) {
    TEST_ASSERT_NOT_NULL(instance);
    TEST_ASSERT_EQUAL(42, instance->getValue());
}

void test_edge_cases(void) {
    instance->setValue(0);
    TEST_ASSERT_EQUAL(0, instance->getValue());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_functionality);
    RUN_TEST(test_edge_cases);
    UNITY_END();
}

void loop() {}
```

**Testing Guidelines:**

- ✅ Test public API behavior
- ✅ Test edge cases (null, zero, max values)
- ✅ Test error conditions
- ✅ Use descriptive test names
- ✅ Keep tests independent (setUp/tearDown)
- ❌ Don't test implementation details
- ❌ Don't rely on test execution order

See [Testing Guide](docs/TESTING.md) for more details.

---

## Pull Request Process

### Before Submitting

1. **Run all tests**
   ```bash
   pio test -e native
   ```

2. **Check compilation**
   ```bash
   pio run -e esp32dev
   ```

3. **Update documentation**
   - Add to [API.md](docs/API.md) if API changed
   - Update [README.md](../README.md) if user-visible
   - Add to [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md) if fixing common issue

4. **Add logging**
   - Use appropriate log levels (ERROR/WARN/INFO/DEBUG)
   - See [Logging System](docs/LOGGING.md)

5. **Format code**
   - Follow existing style
   - 4-space indentation (no tabs)
   - K&R brace style

### Submit Pull Request

1. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Create PR on GitHub**
   - Use descriptive title
   - Explain what changed and why
   - Reference related issues

3. **PR Description Template**
   ```markdown
   ## Description
   Brief description of changes
   
   ## Type of Change
   - [ ] Bug fix
   - [ ] New feature
   - [ ] Documentation update
   - [ ] Code refactoring
   
   ## Testing
   - [ ] Native tests pass
   - [ ] Integration tests pass (if applicable)
   - [ ] Manual testing performed
   
   ## Checklist
   - [ ] Code follows project style
   - [ ] Tests added/updated
   - [ ] Documentation updated
   - [ ] No breaking changes (or documented)
   
   ## Related Issues
   Closes #123
   ```

### Review Process

1. Maintainer reviews PR
2. CI runs automated tests
3. Address feedback if requested
4. Maintainer merges when approved

---

## Coding Standards

### General Principles

- **Readability over cleverness**
- **Explicit over implicit**
- **Consistent with existing code**
- **Self-documenting code** (minimize comments)

### Naming Conventions

```cpp
// Classes: PascalCase
class ButtonStateManager { };

// Methods: camelCase
void getButtonState();

// Private members: _camelCase
private:
    int _currentState;

// Constants: UPPER_CASE
#define MAX_CONNECTIONS 8

// Enums: UPPER_CASE with prefix
enum ButtonState {
    BUTTON_NONE = 0,
    BUTTON_A = 1
};
```

### Code Style

**Indentation:**
```cpp
// 4 spaces, no tabs
void myFunction() {
    if (condition) {
        doSomething();
    }
}
```

**Braces:**
```cpp
// K&R style - opening brace same line
if (condition) {
    doSomething();
} else {
    doSomethingElse();
}

// Functions: opening brace on new line
void myFunction()
{
    // Implementation
}
```

**Line Length:**
- Prefer < 100 characters
- Break long lines at logical points

**Whitespace:**
```cpp
// Spaces around operators
int result = a + b;

// No space after function name
myFunction(param1, param2);

// Space after keywords
if (condition) { }
for (int i = 0; i < n; i++) { }
```

### Comments

**When to Comment:**

```cpp
// ✅ Complex algorithms
// Binary search for matching device in scanned list
int idx = binarySearch(devices, target);

// ✅ Non-obvious behavior
// Battery reports use 0-208 range, not 0-255
uint8_t percentage = (rawValue * 100) / 208;

// ✅ Public API documentation
/**
 * Request battery status update from Wiimote
 * Battery level will be updated asynchronously
 */
void requestBatteryUpdate();
```

**When NOT to Comment:**

```cpp
// ❌ Obvious code
// Set x to 5
int x = 5;

// ❌ Use better names instead
// Loop through all items
for (int i = 0; i < n; i++) { }  // Bad

// Good - no comment needed
for (int itemIndex = 0; itemIndex < itemCount; itemIndex++) { }
```

### Logging

Use appropriate log levels:

```cpp
// Critical failures
if (!initialized) {
    LOG_ERROR("Initialization failed: %s\n", reason);
    return false;
}

// Important milestones
LOG_INFO("Connection established: handle=0x%04x\n", handle);

// Detailed debugging
LOG_DEBUG("Processing packet: type=0x%02x len=%d\n", type, len);
```

See [Logging System](docs/LOGGING.md) for complete guide.

### Error Handling

```cpp
// Check preconditions early
if (data == nullptr) {
    LOG_ERROR("Null data pointer\n");
    return -1;
}

// Provide context in errors
if (queueFull) {
    LOG_ERROR("Queue full: size=%d max=%d\n", size, max);
    return -1;
}

// Clean up on error
if (initFailed) {
    cleanup();
    return false;
}
```

---

## Documentation

### Update Documentation for:

- **New API methods** → [API.md](docs/API.md)
- **Common issues** → [TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)
- **Architecture changes** → [ARCHITECTURE.md](docs/ARCHITECTURE.md)
- **User-visible changes** → [README.md](../README.md)

### Documentation Style

- Use clear, concise language
- Provide code examples
- Explain "why" not just "what"
- Include common pitfalls

---

## Common Tasks

### Adding a New Button

1. Add enum value in button state header
2. Update button parsing in data parser
3. Add test in test_button_state
4. Document in API.md

### Adding a New Sensor

1. Define data structure
2. Add to SensorStateManager
3. Parse in WiimoteDataParser
4. Expose via ESP32Wiimote API
5. Add tests
6. Update documentation

### Fixing a Bug

1. Write failing test that reproduces bug
2. Fix the bug
3. Verify test passes
4. Add log messages if helpful
5. Update troubleshooting guide if common issue

---

## Questions?

- **Code questions**: Check [Architecture](docs/ARCHITECTURE.md)
- **API questions**: See [API Reference](docs/API.md)
- **Testing questions**: Read [Testing Guide](docs/TESTING.md)
- **Other questions**: Open a GitHub issue

---

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (see [LICENSE.md](../LICENSE.md)).

---

Thank you for contributing to ESP32Wiimote! 🎮
