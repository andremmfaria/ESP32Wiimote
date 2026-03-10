# Troubleshooting Guide

Common issues and solutions for ESP32Wiimote.

## Table of Contents

- [Connection Issues](#connection-issues)
- [Compilation Errors](#compilation-errors)
- [Runtime Issues](#runtime-issues)
- [Data Issues](#data-issues)
- [Performance Issues](#performance-issues)

---

## Connection Issues

### Wiimote Won't Connect

**Symptoms:**

- `isConnected()` always returns `false`
- No "Wiimote detected" message in serial output
- Wiimote LEDs keep blinking

**Solutions:**

1. **Check button press timing**

   ```
   - Press 1 + 2 simultaneously
   - Hold for 2-3 seconds
   - Release when LEDs start blinking
   ```

2. **Verify Bluetooth initialization**

   ```cpp
   void setup() {
       Serial.begin(115200);
       wiimote.init();  // Must be called!
   }
   ```

3. **Check serial output for errors**

   ```cpp
   // Define before including ESP32Wiimote.h
   #define WIIMOTE_VERBOSE 3

   #include "ESP32Wiimote.h"
   ```

   Look for:
   - `[ERROR] Bluetooth controller initialization failed!`
   - `[ERROR] HCI: Reset failed`
   - `[ERROR] esp_vhci_host_register_callback failed`

4. **Try power cycling**
   - Remove Wiimote batteries for 10 seconds
   - Power cycle ESP32
   - Try again

5. **Check Wiimote model**
   - Must be "Nintendo RVL-CNT-01"
   - Wii Motion Plus models may not work
   - Some third-party controllers incompatible

---

### Connection Drops Frequently

**Symptoms:**

- Connected but disconnects after a few seconds
- `isConnected()` flickers true/false
- "Wiimote lost" messages

**Solutions:**

1. **Check battery level**

   ```cpp
   uint8_t battery = wiimote.getBatteryLevel();
   Serial.printf("Battery: %d%%\n", battery);
   // Replace if < 20%
   ```

2. **Reduce distance**
   - Keep within 3-5 meters
   - Remove obstacles between devices
   - Avoid metal interference

3. **Call task() regularly**

   ```cpp
   void loop() {
       wiimote.task();  // MUST be called!
       // Don't add long delays here
   }
   ```

4. **Check for long blocking operations**

   ```cpp
   // Bad: Blocks task() from running
   void loop() {
       wiimote.task();
       delay(1000);  // Too long!
   }
   
   // Good: Non-blocking timing
   static unsigned long lastCheck = 0;
   if (millis() - lastCheck > 1000) {
       // Do something
       lastCheck = millis();
   }
   ```

---

### Can't Find Wiimote After First Connection

**Symptoms:**

- Works once, then never connects again
- ESP32 reset doesn't help

**Solution:**

Remove Wiimote sync:

```
1. Open Wiimote battery cover
2. Press red SYNC button (inside battery compartment)
3. Close cover
4. Try connecting again (1 + 2)
```

---

## Compilation Errors

### `ESP32Wiimote.h: No such file or directory`

**Cause:** Library not installed correctly

**Solution:**

```bash
# Arduino IDE
Sketch > Include Library > Add .ZIP Library...

# PlatformIO
lib_deps = 
    https://github.com/user/ESP32Wiimote.git
```

---

### `'LOG_ERROR' was not declared`

**Cause:** Missing logging header include

**Solution:**

```cpp
#include <ESP32Wiimote.h>
```

---

### `undefined reference to 'setup()'` / `'loop()'`

**Cause:** Compiling library as standalone (missing main sketch)

**Solution:**

For ESP32 builds, you need a sketch file:

```cpp
// main.cpp or YourSketch.ino
#include <ESP32Wiimote.h>

ESP32Wiimote wiimote;

void setup() {
    Serial.begin(115200);
    wiimote.init();
}

void loop() {
    wiimote.task();
}
```

For native tests, this is expected - tests provide setup/loop.

---

### `multiple definition of 'TinyWiimoteInit'`

**Cause:** Including .cpp files instead of .h files

**Solution:**

```cpp
// Wrong
#include "TinyWiimote.cpp"

// Correct
#include "TinyWiimote.h"
```

---

## Runtime Issues

### No Serial Output

**Symptoms:**

- Serial monitor empty
- No logs appear

**Solutions:**

1. **Check serial initialization**

   ```cpp
   void setup() {
       Serial.begin(115200);  // Must be called first!
       delay(100);            // Give serial time to init
       wiimote.init();
   }
   ```

2. **Check baud rate**

   ```
   Serial Monitor must be set to 115200 baud
   ```

3. **Check log level**

   ```cpp
   // In your sketch before including the library
   #define WIIMOTE_VERBOSE 2  // Increase to 2 or 3

   #include "ESP32Wiimote.h"
   ```

4. **Try USB cable**
   - Some cables are power-only
   - Use a data-capable cable

---

### ESP32 Crashes/Reboots

**Symptoms:**

- ESP32 resets during operation
- "Brownout detector" messages
- Watchdog timeout errors

**Solutions:**

1. **Check power supply**
   - Use 500mA+ capable PSU
   - Avoid USB hub power
   - Try different USB port

2. **Reduce log level**

   ```cpp
   #define WIIMOTE_VERBOSE 1  // Less serial traffic

   #include "ESP32Wiimote.h"
   ```

3. **Check stack size**

   ```cpp
   // In platformio.ini, if needed
   board_build.f_cpu = 240000000L
   board_build.f_flash = 80000000L
   ```

4. **Check for infinite loops**

   ```cpp
   void loop() {
       wiimote.task();  // Must not block
       // Avoid while(true) without task() call
   }
   ```

---

### Memory Allocation Failures

**Symptoms:**

- `[ERROR] malloc failed` logs
- Crashes under load

**Solutions:**

1. **Check available heap**

   ```cpp
   void loop() {
       static unsigned long lastCheck = 0;
       if (millis() - lastCheck > 5000) {
           Serial.printf("Free heap: %d bytes\n", 
                         ESP.getFreeHeap());
           lastCheck = millis();
       }
   }
   ```

2. **Reduce queue sizes**

   ```cpp
   // In ESP32Wiimote constructor
   _queueManager = new HciQueueManager(16, 16);  // Smaller queues
   ```

3. **Avoid dynamic allocation in loop**

   ```cpp
   // Bad
   void loop() {
       String msg = "Button pressed";  // Heap allocation
   }
   
   // Good
   void loop() {
       static char msg[32];
       sprintf(msg, "Button pressed");
   }
   ```

---

## Data Issues

### Battery Always Shows 0%

**Symptoms:**

- `getBatteryLevel()` returns 0
- Battery never updates

**Solutions:**

1. **Request battery update**

   ```cpp
   void loop() {
       if (wiimote.isConnected()) {
           static bool requested = false;
           if (!requested) {
               wiimote.requestBatteryUpdate();
               requested = true;
           }
       }
   }
   ```

2. **Check connection**

   ```cpp
   if (wiimote.isConnected()) {
       uint8_t battery = wiimote.getBatteryLevel();
       Serial.printf("Battery: %d%%\n", battery);
   }
   ```

3. **Wait for response**
   - Battery update is asynchronous
   - May take 100-500ms after request
   - Check again after short delay

---

### Buttons Not Responding

**Symptoms:**

- `getButtonState()` always returns `BUTTON_NONE`
- No button events detected

**Solutions:**

1. **Check available() return**

   ```cpp
   if (wiimote.available()) {  // Must check this!
       ButtonState btn = wiimote.getButtonState();
   }
   ```

2. **Check filters**

   ```cpp
   // If you set this, buttons are ignored!
   wiimote.addFilter(ACTION_IGNORE, FILTER_BUTTON);
   ```

3. **Check button comparison**

   ```cpp
   // Wrong: Checks exact match
   if (btn == BUTTON_A && btn == BUTTON_B) {  // Never true!
   
   // Correct: Checks presence
   if ((btn & BUTTON_A) && (btn & BUTTON_B)) {
   ```

4. **Enable debug logging**

   ```cpp
   #define WIIMOTE_VERBOSE 3

   #include "ESP32Wiimote.h"
   // Watch for "BTCODE_HID" in logs
   ```

---

### Accelerometer Data All Zeros

**Symptoms:**

- `getAccelState()` returns 0,0,0
- No motion detected

**Solutions:**

1. **Enable accelerometer mode**

   ```cpp
   // Accelerometer reporting must be enabled
   // Library does this automatically on connection
   ```

2. **Check reporting mode**
   - Mode 0x30: Buttons only (no accel)
   - Mode 0x31: Buttons + accelerometer ✓
   - Mode 0x35: Buttons + accel + extensions ✓

3. **Wait for connection**

   ```cpp
   if (!wiimote.isConnected()) {
       return;  // No data when disconnected
   }
   ```

4. **Check for data updates**

   ```cpp
   if (wiimote.available()) {
       AccelState accel = wiimote.getAccelState();
       Serial.printf("Accel: %d,%d,%d\n", 
           accel.xAxis, accel.yAxis, accel.zAxis);
   }
   ```

---

### Nunchuk Not Detected

**Symptoms:**

- `getNunchukState()` returns default values
- "Extension controller NOT connected" log

**Solutions:**

1. **Check physical connection**
   - Nunchuk firmly inserted
   - Connector clean (no dust)
   - Try another Nunchuk

2. **Check timing**
   - Extension detection happens after connection
   - May take 1-2 seconds
   - Look for "Extension controller connected" log

3. **Enable debug logging**

   ```cpp
   #define WIIMOTE_VERBOSE 3
   // Look for extension detection sequence
   ```

4. **Check reporting mode**
   - Mode 0x35 (with extensions) used when Nunchuk detected
   - Library handles this automatically

---

## Performance Issues

### High CPU Usage

**Symptoms:**

- ESP32 runs hot
- Other tasks slow down

**Solutions:**

1. **Reduce log level**

   ```cpp
   #define WIIMOTE_VERBOSE 1  // Less logging
   ```

2. **Add small delays**

   ```cpp
   void loop() {
       wiimote.task();
       // ... processing ...
       delay(1);  // Small yield to FreeRTOS
   }
   ```

3. **Rate-limit data processing**

   ```cpp
   if (wiimote.available()) {
       static unsigned long lastProcess = 0;
       if (millis() - lastProcess > 10) {  // Max 100 Hz
           // Process data
           lastProcess = millis();
       }
   }
   ```

---

### Slow Button Response

**Symptoms:**

- Button presses delayed
- Laggy input

**Solutions:**

1. **Call task() more frequently**

   ```cpp
   void loop() {
       wiimote.task();  // Top of loop, no long delays after
       
       if (wiimote.available()) {
           // Handle input
       }
   }
   ```

2. **Remove long delays**

   ```cpp
   // Bad
   void loop() {
       wiimote.task();
       doSomething();
       delay(100);  // Adds latency!
   }
   
   // Good
   void loop() {
       wiimote.task();
       
       static unsigned long last = 0;
       if (millis() - last > 100) {
           doSomething();
           last = millis();
       }
   }
   ```

3. **Profile your code**

   ```cpp
   unsigned long start = micros();
   expensiveFunction();
   Serial.printf("Took: %lu us\n", micros() - start);
   ```

---

## Testing Issues

### Native Tests Fail to Compile

**Symptoms:**

- `Arduino.h: No such file`
- Undefined symbols during native test compilation

**Solution:**

Add platform guards:

```cpp
#ifndef NATIVE_TEST
    #include <Arduino.h>
#else
    // Mock definitions
    #define Serial MockSerial
#endif
```

Or use existing guards in library headers.

---

### Integration Tests Can't See Output

**Symptoms:**

- Test prompts don't appear
- Action messages missing

**Solution:**

Use `-v` flag with PlatformIO:

```bash
pio test -e esp32dev -v
```

Without `-v`, PlatformIO filters test output.

---

## Getting Help

Still stuck? Try these resources:

1. **Enable full logging**

   ```cpp
   #define WIIMOTE_VERBOSE 3
   ```

2. **Check documentation**
   - [API Reference](API.md)
   - [Architecture](ARCHITECTURE.md)
   - [Logging System](LOGGING.md)

3. **Search issues**
   - GitHub Issues page
   - Look for similar problems

4. **Create minimal reproduction**

   ```cpp
   #include <ESP32Wiimote.h>
   ESP32Wiimote wiimote;
   void setup() {
       Serial.begin(115200);
       wiimote.init();
   }
   void loop() {
       wiimote.task();
   }
   ```

5. **Include diagnostic info**
   - ESP32 board model
   - Arduino/ESP32 core version
   - PlatformIO version
   - Full serial output
   - Minimal code example

---

## See Also

- [API Reference](API.md) - Complete API documentation
- [Testing Guide](TESTING.md) - Running tests
- [Logging System](LOGGING.md) - Debug configuration
- [Architecture](ARCHITECTURE.md) - System internals
