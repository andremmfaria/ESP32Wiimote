#include "ESP32Wiimote.h"
#include "serial/serial_command_dispatcher.h"
#include "serial/serial_command_parser.h"
#include "serial/serial_command_session.h"

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

struct EmbeddedSerialTarget : SerialCommandTarget {
    bool allowWifiTokenMutation{false};

    bool setLeds(uint8_t ledMask) override { return wiimote.setLeds(ledMask); }
    bool setReportingMode(uint8_t mode, bool continuous) override {
        return wiimote.setReportingMode(static_cast<ReportingMode>(mode), continuous);
    }
    bool setAccelerometerEnabled(bool enabled) override {
        return wiimote.setAccelerometerEnabled(enabled);
    }
    bool requestStatus() override { return wiimote.requestStatus(); }
    void setScanEnabled(bool enabled) override { wiimote.setScanEnabled(enabled); }
    bool startDiscovery() override { return wiimote.startDiscovery(); }
    bool stopDiscovery() override { return wiimote.stopDiscovery(); }
    bool disconnectActiveController(uint8_t reason) override {
        return wiimote.disconnectActiveController(
            static_cast<ESP32Wiimote::DisconnectReason>(reason));
    }
    void setAutoReconnectEnabled(bool enabled) override {
        wiimote.setAutoReconnectEnabled(enabled);
    }
    void clearReconnectCache() override { wiimote.clearReconnectCache(); }

    bool setWifiControlEnabled(bool enabled) override {
        wiimote.enableWifiControl(enabled, wiimote.getConfig().wifi.deliveryMode);
        return wiimote.isWifiControlEnabled() == enabled;
    }
    bool setWifiDeliveryMode(uint8_t mode) override {
        if (mode > static_cast<uint8_t>(WifiDeliveryMode::RestAndWebSocket)) {
            return false;
        }
        return wiimote.setWifiDeliveryMode(static_cast<WifiDeliveryMode>(mode));
    }
    bool setWifiNetworkCredentials(const char *ssid, const char *password) override {
        return wiimote.updateWifiNetworkCredentials(ssid, password);
    }
    bool restartWifiControl() override { return wiimote.restartWifiControl(); }
    bool setWifiApiToken(const char *token) override { return wiimote.updateWifiApiToken(token); }
    bool isWifiApiTokenMutationAllowed() const override { return allowWifiTokenMutation; }

    bool isConnected() const override { return wiimote.isConnected(); }
    uint8_t getBatteryLevel() const override { return wiimote.getBatteryLevel(); }
};

static SerialDispatchResult dispatchSerialLine(const char *line,
                                               EmbeddedSerialTarget *target,
                                               SerialCommandSession *session,
                                               bool requireUnlock,
                                               uint32_t nowMs) {
    SerialParsedCommand parsed;
    const SerialParseResult parseResult = serialCommandParse(line, &parsed);
    TEST_ASSERT_EQUAL(SerialParseResult::Ok, parseResult);

    SerialDispatchOptions options = {};
    options.session = session;
    options.privilegedCommandsRequireUnlock = requireUnlock;
    options.nowMs = nowMs;

    return serialCommandDispatch(parsed, target, options);
}

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

// Test: Wi-Fi runtime command surface updates credentials/token and reports config
void testWifiRuntimeConfigCommands() {
    TEST_PRINT("\n=== WIFI RUNTIME CONFIG COMMANDS TEST ===");

    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_token_v1";
    cfg.auth.wifiApiToken = "wifi_token_v1";
    cfg.wifi.enabled = true;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;
    cfg.wifi.network = {"wifi_ssid_v1", "wifi_pass_v1"};

    wiimote.configure(cfg);

    TEST_ASSERT_TRUE_MESSAGE(wiimote.updateWifiNetworkCredentials("wifi_ssid_v2", "wifi_pass_v2"),
                             "updateWifiNetworkCredentials should accept valid non-empty values");
    TEST_ASSERT_EQUAL_STRING("wifi_ssid_v2", wiimote.getConfig().wifi.network.ssid);
    TEST_ASSERT_EQUAL_STRING("wifi_pass_v2", wiimote.getConfig().wifi.network.password);

    TEST_ASSERT_TRUE_MESSAGE(wiimote.updateWifiApiToken("wifi_token_v2"),
                             "updateWifiApiToken should accept valid token");
    TEST_ASSERT_TRUE_MESSAGE(wiimote.hasWifiApiToken(), "hasWifiApiToken should return true");
    TEST_ASSERT_EQUAL_STRING("wifi_token_v2", wiimote.getConfig().auth.wifiApiToken);

    TEST_ASSERT_FALSE_MESSAGE(wiimote.updateWifiNetworkCredentials("", "x"),
                              "Empty SSID must be rejected");
    TEST_ASSERT_FALSE_MESSAGE(wiimote.updateWifiApiToken(""),
                              "Empty Wi-Fi API token must be rejected");
}

// Test: Wi-Fi runtime lifecycle command surface (mode switch + restart)
void testWifiRuntimeLifecycleCommands() {
    TEST_PRINT("\n=== WIFI RUNTIME LIFECYCLE COMMANDS TEST ===");

    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_token_lc";
    cfg.auth.wifiApiToken = "wifi_token_lc";
    cfg.wifi.enabled = true;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;
    cfg.wifi.network = {"wifi_ssid_lc", "wifi_pass_lc"};
    wiimote.configure(cfg);

    wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    TEST_ASSERT_TRUE_MESSAGE(wiimote.isWifiControlEnabled(), "Wi-Fi control should be enabled");

    for (int i = 0; i < 6; ++i) {
        wiimote.task();
        delay(5);
    }

    ESP32Wiimote::WifiControlState state = wiimote.getWifiControlState();
    TEST_ASSERT_TRUE_MESSAGE(state.ready, "Wi-Fi control should become ready in RestOnly mode");
    TEST_ASSERT_FALSE_MESSAGE(state.websocketRoutesRegistered,
                              "RestOnly mode should not register WebSocket routes");

    TEST_ASSERT_TRUE_MESSAGE(wiimote.setWifiDeliveryMode(WifiDeliveryMode::RestAndWebSocket),
                             "setWifiDeliveryMode should accept valid mode");
    state = wiimote.getWifiControlState();
    TEST_ASSERT_FALSE_MESSAGE(state.ready,
                              "Mode switch should restart lifecycle and clear ready state");

    for (int i = 0; i < 7; ++i) {
        wiimote.task();
        delay(5);
    }

    state = wiimote.getWifiControlState();
    TEST_ASSERT_TRUE_MESSAGE(state.ready,
                             "Wi-Fi control should become ready in RestAndWebSocket mode");
    TEST_ASSERT_TRUE_MESSAGE(state.websocketRoutesRegistered,
                             "RestAndWebSocket mode should register WebSocket routes");

    TEST_ASSERT_TRUE_MESSAGE(wiimote.restartWifiControl(),
                             "restartWifiControl should succeed when control is enabled");
    state = wiimote.getWifiControlState();
    TEST_ASSERT_FALSE_MESSAGE(state.ready,
                              "Restart should clear ready state until lifecycle progresses again");
}

// Test: serial interface enforces unlock for privileged Wi-Fi mutation commands
void testSerialInterfaceWifiCommandsRequireUnlock() {
    TEST_PRINT("\n=== SERIAL INTERFACE LOCK MODEL TEST ===");

    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_unlock_token";
    cfg.auth.wifiApiToken = "wifi_serial_token";
    cfg.wifi.enabled = true;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;
    cfg.wifi.network = {"serial_wifi_ssid", "serial_wifi_pass"};
    wiimote.configure(cfg);

    EmbeddedSerialTarget target;
    SerialCommandSession session;
    session.setToken("serial_unlock_token");

    TEST_ASSERT_EQUAL(SerialDispatchResult::Locked,
                      dispatchSerialLine("wm wifi-control on", &target, &session, true, 1000U));

    TEST_ASSERT_EQUAL(
        SerialDispatchResult::Ok,
        dispatchSerialLine("wm unlock serial_unlock_token 30", &target, &session, true, 1100U));

    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok,
                      dispatchSerialLine("wm wifi-control on", &target, &session, true, 1200U));
    TEST_ASSERT_TRUE_MESSAGE(wiimote.isWifiControlEnabled(),
                             "Serial wifi-control on should enable Wi-Fi lifecycle");
}

// Test: serial interface updates Wi-Fi credentials through dispatcher
void testSerialInterfaceWifiSetNetworkCommand() {
    TEST_PRINT("\n=== SERIAL INTERFACE WIFI SET NETWORK TEST ===");

    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_unlock_token2";
    cfg.auth.wifiApiToken = "wifi_serial_token2";
    cfg.wifi.enabled = true;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;
    cfg.wifi.network = {"ssid_old", "pass_old"};
    wiimote.configure(cfg);

    EmbeddedSerialTarget target;
    SerialCommandSession session;
    session.setToken("serial_unlock_token2");

    TEST_ASSERT_EQUAL(
        SerialDispatchResult::Ok,
        dispatchSerialLine("wm unlock serial_unlock_token2 30", &target, &session, true, 2000U));

    TEST_ASSERT_EQUAL(SerialDispatchResult::Ok,
                      dispatchSerialLine("wm wifi-set-network ssid_new pass_new", &target, &session,
                                         true, 2100U));

    TEST_ASSERT_EQUAL_STRING("ssid_new", wiimote.getConfig().wifi.network.ssid);
    TEST_ASSERT_EQUAL_STRING("pass_new", wiimote.getConfig().wifi.network.password);
}

// Test: serial interface policy-gates Wi-Fi token mutation
void testSerialInterfaceWifiTokenPolicy() {
    TEST_PRINT("\n=== SERIAL INTERFACE WIFI TOKEN POLICY TEST ===");

    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_unlock_token3";
    cfg.auth.wifiApiToken = "wifi_serial_token3";
    cfg.wifi.enabled = true;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;
    cfg.wifi.network = {"ssid_policy", "pass_policy"};
    wiimote.configure(cfg);

    EmbeddedSerialTarget target;
    SerialCommandSession session;
    session.setToken("serial_unlock_token3");

    TEST_ASSERT_EQUAL(
        SerialDispatchResult::Ok,
        dispatchSerialLine("wm unlock serial_unlock_token3 30", &target, &session, true, 3000U));

    target.allowWifiTokenMutation = false;
    TEST_ASSERT_EQUAL(
        SerialDispatchResult::PolicyBlocked,
        dispatchSerialLine("wm wifi-set-token blocked_token", &target, &session, true, 3100U));

    target.allowWifiTokenMutation = true;
    TEST_ASSERT_EQUAL(
        SerialDispatchResult::Ok,
        dispatchSerialLine("wm wifi-set-token allowed_token", &target, &session, true, 3200U));
    TEST_ASSERT_EQUAL_STRING("allowed_token", wiimote.getConfig().auth.wifiApiToken);
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
    RUN_TEST(testWifiRuntimeConfigCommands);
    RUN_TEST(testWifiRuntimeLifecycleCommands);
    RUN_TEST(testSerialInterfaceWifiCommandsRequireUnlock);
    RUN_TEST(testSerialInterfaceWifiSetNetworkCommand);
    RUN_TEST(testSerialInterfaceWifiTokenPolicy);

    UNITY_END();

    TEST_PRINT("\n=== All tests complete ===");
}

void loop() {
    // Keep processing Wiimote tasks
    wiimote.task();
    delay(10);
}
