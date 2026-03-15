#include "ESP32Wiimote.h"

#include <Arduino.h>

#include <unity.h>

/**
 * Integration test for ESP32Wiimote - requires actual hardware
 *
 * IMPORTANT: Run with -v flag to see user instructions!
 *
 * Command:
 *   pio test -e esp32dev --upload-port /dev/ttyUSB0 -f embedded/test_integration -v
 *
 * Setup:
 * 1. Upload this test to ESP32
 * 2. Run with -v flag to see instructions
 * 3. Press 1+2 on Wiimote when prompted
 * 4. Follow on-screen instructions for button tests
 */

// Helper macro for test messages
// NOTE: These messages require the -v (verbose) flag to be visible during test execution
// Run with: pio test -e esp32dev --upload-port /dev/ttyUSB0 -f embedded/test_integration -v
#define TEST_PRINT(msg) Serial.println(msg)

ESP32Wiimote wiimote;
bool connectionTested = false;

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

// Test: Bluetooth initialization
void testBluetoothInit() {
    TEST_PRINT("Testing Bluetooth initialization...");
    wiimote.init();
    // If we get here without crash, BT init succeeded
    TEST_ASSERT_TRUE(true);
}

// Test: Initial connection state
void testInitialConnectionState() {
    TEST_ASSERT_FALSE_MESSAGE(wiimote.isConnected(), "Should not be connected initially");
}

// Test: Connection attempt (requires user action)
void testWiimoteConnection() {
    if (connectionTested) {
        TEST_ASSERT_TRUE(true);  // Already tested
        return;
    }

    TEST_PRINT("\n=== WIIMOTE CONNECTION TEST ===");
    TEST_PRINT(">>> ACTION REQUIRED: Please press 1+2 on your Wiimote NOW <<<");
    TEST_PRINT("Waiting 60 seconds for connection...");

    unsigned long startTime = millis();
    bool connected = false;

    while (millis() - startTime < 60000) {  // 60 second timeout
        wiimote.task();

        if (ESP32Wiimote::isConnected()) {
            connected = true;
            TEST_PRINT(">>> Wiimote connected successfully! <<<");
            break;
        }

        delay(100);
    }

    connectionTested = true;
    TEST_ASSERT_TRUE_MESSAGE(connected, "Failed to connect to Wiimote in 60 seconds");
}

// Test: Battery level reading (requires connection)
void testBatteryLevel() {
    if (!ESP32Wiimote::isConnected()) {
        TEST_IGNORE_MESSAGE("Wiimote not connected, skipping battery test");
        return;
    }

    // Give it time to get battery data
    for (int i = 0; i < 20; i++) {
        wiimote.task();
        delay(50);
    }

    uint8_t battery = ESP32Wiimote::getBatteryLevel();
    char battMsg[50];
    snprintf(battMsg, sizeof(battMsg), "Battery level: %d%%", battery);
    TEST_PRINT(battMsg);

    // Battery should be between 0 and 100%
    TEST_ASSERT_TRUE_MESSAGE(battery >= 0 && battery <= 100, "Battery level out of range");
}

// Test: Button press detection (requires user action)
void testButtonPress() {
    if (!ESP32Wiimote::isConnected()) {
        TEST_IGNORE_MESSAGE("Wiimote not connected, skipping button test");
        return;
    }

    TEST_PRINT("\n=== BUTTON TEST ===");
    TEST_PRINT(">>> ACTION REQUIRED: Press button A on Wiimote within 10 seconds <<<");

    unsigned long startTime = millis();
    bool buttonPressed = false;

    while (millis() - startTime < 10000) {
        wiimote.task();

        if (wiimote.available() > 0) {
            ButtonState button = wiimote.getButtonState();

            if (buttonStateHas(button, kButtonA)) {
                buttonPressed = true;
                TEST_PRINT(">>> Button A detected! <<<");
                break;
            }
        }

        delay(50);
    }

    TEST_ASSERT_TRUE_MESSAGE(buttonPressed, "Button A not pressed in time");
}

// Test: Accelerometer data (requires connection)
void testAccelerometerData() {
    if (!ESP32Wiimote::isConnected()) {
        TEST_IGNORE_MESSAGE("Wiimote not connected, skipping accelerometer test");
        return;
    }

    TEST_PRINT("\n=== ACCELEROMETER TEST ===");
    TEST_PRINT("Reading accelerometer data...");

    // Give it time to get accel data
    for (int i = 0; i < 50; i++) {
        wiimote.task();
        delay(20);
    }

    AccelState accel = wiimote.getAccelState();

    char accelMsg[100];
    snprintf(accelMsg, sizeof(accelMsg), "Accel: X=%d, Y=%d, Z=%d", accel.xAxis, accel.yAxis,
             accel.zAxis);
    TEST_PRINT(accelMsg);

    // Accelerometer values should be reasonable (not all zeros in normal use)
    // At rest, at least one axis should show gravity (~130-150 range)
    bool hasReasonableValues = (accel.xAxis > 0 || accel.yAxis > 0 || accel.zAxis > 0);
    TEST_ASSERT_TRUE_MESSAGE(hasReasonableValues, "Accelerometer returned all zeros");
}

// Test: Connection stability
void testConnectionStability() {
    if (!ESP32Wiimote::isConnected()) {
        TEST_IGNORE_MESSAGE("Wiimote not connected, skipping stability test");
        return;
    }

    TEST_PRINT("\n=== CONNECTION STABILITY TEST ===");
    TEST_PRINT("Testing connection for 5 seconds...");

    bool stayedConnected = true;

    for (int i = 0; i < 50; i++) {
        wiimote.task();

        if (!ESP32Wiimote::isConnected()) {
            stayedConnected = false;
            break;
        }

        delay(100);
    }

    TEST_ASSERT_TRUE_MESSAGE(stayedConnected, "Connection dropped during stability test");
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for serial

    TEST_PRINT("\n\n=================================");
    TEST_PRINT("ESP32Wiimote Integration Tests");
    TEST_PRINT("=================================\n");
    TEST_PRINT("NOTE: Some tests require user interaction with the Wiimote");
    TEST_PRINT("Follow on-screen instructions when prompted\n");

    UNITY_BEGIN();

    RUN_TEST(testBluetoothInit);
    RUN_TEST(testInitialConnectionState);
    RUN_TEST(testWiimoteConnection);
    RUN_TEST(testBatteryLevel);
    RUN_TEST(testButtonPress);
    RUN_TEST(testAccelerometerData);
    RUN_TEST(testConnectionStability);

    UNITY_END();

    TEST_PRINT("\n=== All tests complete ===");
}

void loop() {
    // Keep processing Wiimote tasks
    wiimote.task();
    delay(10);
}
