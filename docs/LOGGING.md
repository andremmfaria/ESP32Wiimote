# Logging System

ESP32Wiimote includes a comprehensive 4-level logging system for debugging and diagnostics.

## Quick Start

Set logging level in `src/utils/serial_logging.h`:

```cpp
#define WIIMOTE_VERBOSE 2  // 0=Errors, 1=+Warnings, 2=+Info, 3=+Debug
```

## Log Levels

### Level 0: ERROR (Always Shown)

**What:** Critical failures that prevent operation
**When to use:** Initialization failures, null pointers, connection errors

**Example output:**
```
[ERROR] HCI: Reset failed with status=0x05
[ERROR] L2CAP: sendCallback is null
[ERROR] Bluetooth controller initialization failed!
```

**Code example:**
```cpp
if (!initialized) {
    LOG_ERROR("Component initialization failed\n");
    return false;
}
```

---

### Level 1: WARNING (+ Errors)

**What:** Non-critical issues that should be noted
**When to use:** Capacity limits, fallback behavior, deprecated features

**Example output:**
```
[WARN] L2CAP: Connection table full, cannot add connection
[WARN] HciCallback: Queue manager not set, cannot send packet
[WARN] TinyWiimote: Cannot request battery update - not connected
```

**Code example:**
```cpp
if (queueFull) {
    LOG_WARN("Queue at capacity, dropping packet\n");
}
```

---

### Level 2: INFO (+ Warnings + Errors) **[DEFAULT]**

**What:** Important milestones and state changes
**When to use:** Connections, detections, initialization complete

**Example output:**
```
[INFO] ESP32Wiimote: Starting initialization...
[INFO] HCI: Device initialized, starting inquiry
[INFO] HCI: Wiimote detected! Requesting remote name...
[INFO] TinyWiimote: Wiimote detected! Setting LED and marking as connected
[INFO] Wiimote detected
[INFO] L2CAP: Connection established successfully
[INFO] Extension controller connected
[INFO] Nunchuk detected
```

**Code example:**
```cpp
LOG_INFO("Connection established to device 0x%04x\n", handle);
```

---

### Level 3: DEBUG (Everything)

**What:** Detailed traces, function calls, packet dumps
**When to use:** Deep debugging, protocol analysis, packet inspection

**Example output:**
```
[DEBUG] TinyWiimote: Initializing TinyWiimote core...
[DEBUG] TinyWiimote: Resetting wiimote state...
[DEBUG] TinyWiimote: Setting up packet sender...
[DEBUG] HCI: Reset successful
[DEBUG] HCI: Inquiry result: found 1 device(s)
[DEBUG] HCI: Remote name: Nintendo RVL-CNT-01
[DEBUG] wiimote_set_leds
[DEBUG] queued acl_l2cap_packet(Set LEDs, leds=0x01)
[DEBUG] L2CAP: Sending connection request: ch=0x000b psm=0x0013 cid=0x0045
[DEBUG] SEND => 02 0b 20 0c 00 08 00 01 00 02 01 04 00 13 00 45 00
[DEBUG] RECV <= 04 0e 04 01 03 0c 00
```

**Code example:**
```cpp
LOG_DEBUG("Processing packet: type=0x%02x len=%d\n", type, len);
```

---

## Configuration

Edit `src/utils/serial_logging.h`:

```cpp
/**
 * Logging Level Configuration
 * 
 * Set WIIMOTE_VERBOSE to control logging verbosity:
 *   0 = Errors only (always shown)
 *   1 = Errors + Warnings
 *   2 = Errors + Warnings + Info (default)
 *   3 = Errors + Warnings + Info + Debug (full verbose)
 */
#define WIIMOTE_VERBOSE 2
```

### For Development

```cpp
#define WIIMOTE_VERBOSE 3  // See everything
```

### For Production

```cpp
#define WIIMOTE_VERBOSE 0  // Only errors
```

### For User-Facing Applications

```cpp
#define WIIMOTE_VERBOSE 1  // Errors + Warnings
```

---

## Using Log Macros

### In Your Code

```cpp
#include "utils/serial_logging.h"

void myFunction() {
    LOG_ERROR("Critical: %s\n", errorMsg);
    LOG_WARN("Potential issue: retrying...\n");
    LOG_INFO("Connection established\n");
    LOG_DEBUG("Variable x=%d, y=%d\n", x, y);
}
```

### Macro Signatures

```cpp
LOG_ERROR(format, ...)   // Always shown
LOG_WARN(format, ...)    // Level 1+
LOG_INFO(format, ...)    // Level 2+
LOG_DEBUG(format, ...)   // Level 3 only
```

**Note:** All macros use `printf`-style formatting.

---

## Common Patterns

### Conditional Logging

```cpp
if (result != SUCCESS) {
    LOG_ERROR("Operation failed: code=%d\n", result);
    return false;
}
LOG_INFO("Operation successful\n");
```

### Hex Dumps

```cpp
LOG_DEBUG("Packet data: %s\n", format2Hex(buffer, length));
```

### Connection Events

```cpp
LOG_INFO("Wiimote connected! Handle: 0x%04x\n", handle);
// ... later ...
LOG_INFO("Wiimote disconnected! Reason: 0x%02x\n", reason);
```

### Battery Updates

```cpp
uint8_t rawBattery = data[7];
LOG_DEBUG("Wiimote: Status report 0x20 - Raw battery value: 0x%02x (%d)\n", 
          rawBattery, rawBattery);
uint8_t percentage = (rawBattery * 100) / 208;
LOG_INFO("Battery level: %d%%\n", percentage);
```

---

## Log Output Examples

### Level 0: ERROR Only

Minimal output, only critical failures:
```
[ERROR] Bluetooth controller initialization failed!
```

### Level 1: WARN

Adds warnings:
```
[WARN] HCI packet queue at 90% capacity
[ERROR] Failed to allocate memory for packet
```

### Level 2: INFO (Default)

Shows the story of what's happening:
```
[INFO] ESP32Wiimote: Starting initialization...
[INFO] HCI: Device initialized, starting inquiry
[INFO] HCI: Wiimote detected! Requesting remote name...
[INFO] HCI: Nintendo Wiimote confirmed! Initiating connection...
[INFO] HCI: Connection complete! Handle: 0x000b
[INFO] TinyWiimote: ACL connection established! Handle: 0x000b
[INFO] L2CAP: Connection established successfully
[INFO] TinyWiimote: Wiimote detected! Setting LED and marking as connected
[INFO] Wiimote detected
[INFO] Extension controller NOT connected
```

### Level 3: DEBUG (Full Verbose)

All internal operations visible:
```
[DEBUG] TinyWiimote: Initializing TinyWiimote core...
[DEBUG] TinyWiimote: Resetting wiimote state...
[DEBUG] TinyWiimote: Setting up packet sender...
[DEBUG] TinyWiimote: Initializing L2CAP connections...
[DEBUG] TinyWiimote: Initializing Wiimote protocol...
[DEBUG] TinyWiimote: Initializing Wiimote extensions...
[DEBUG] TinyWiimote: Initializing HCI events...
[INFO] TinyWiimote: Core initialization complete!
[DEBUG] HciQueue: Creating TX and RX queues...
[DEBUG] HciQueue: Queues created successfully
[DEBUG] Bluetooth: Starting Bluetooth controller...
[INFO] Bluetooth: Controller started, VHCI available
[DEBUG] esp_vhci_host_check_send_available=1
[DEBUG] SEND => 01 03 0c 00
[DEBUG] notifyHostRecv: 04 0e 04 01 03 0c 00
... (much more detail)
```

---

## Troubleshooting

### "No output at all"

Check:
1. Serial initialized: `Serial.begin(115200);`
2. Log level set correctly in `serial_logging.h`
3. Recompile after changing log level

### "Too much output"

Lower the log level:
```cpp
#define WIIMOTE_VERBOSE 1  // or 0
```

### "Missing some logs"

Raise the log level:
```cpp
#define WIIMOTE_VERBOSE 3
```

### "Compile-time errors about LOG_*"

Include the header:
```cpp
#include "utils/serial_logging.h"
```

---

## Performance Impact

| Level | Impact | Use Case |
|-------|--------|----------|
| 0 (ERROR) | Minimal | Production |
| 1 (WARN) | Very Low | Production with monitoring |
| 2 (INFO) | Low | Development, demos |
| 3 (DEBUG) | Moderate | Deep debugging only |

**Notes:**
- Disabled log macros compile to `do {} while(0)` (zero overhead)
- Level 3 generates significant serial traffic
- Use level 3 only when needed for specific debugging

---

## Best Practices

### ✅ Do

- Use appropriate log levels (ERROR for failures, INFO for milestones)
- Include context in messages (handles, addresses, values)
- End messages with `\n`
- Use descriptive prefixes for subsystems

### ❌ Don't

- Log in tight loops without rate limiting
- Use DEBUG for everything
- Include sensitive data in production logs
- Mix log levels (e.g., INFO messages with ERROR macro)

---

## Example: Adding Logging to New Code

```cpp
// Bad: No context
LOG_DEBUG("Failed\n");

// Good: Clear context
LOG_ERROR("Failed to initialize sensor: error code %d\n", errorCode);

// Bad: Wrong level
LOG_DEBUG("Wiimote connected\n");  // Should be INFO

// Good: Appropriate level
LOG_INFO("Wiimote connected! Handle: 0x%04x\n", handle);

// Bad: No newline
LOG_INFO("Processing packet");

// Good: Proper formatting
LOG_INFO("Processing packet: type=0x%02x len=%d\n", type, len);
```

---

## See Also

- [API Reference](API.md) - Complete API documentation
- [Testing Guide](TESTING.md) - Running unit tests
- [Architecture](ARCHITECTURE.md) - System internals
