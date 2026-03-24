#include "ESP32Wiimote.h"

#include <Arduino.h>

#include <unity.h>

namespace {

void pumpTasks(ESP32Wiimote &wiimote, const int iterations, const int delayMs) {
    for (int i = 0; i < iterations; ++i) {
        wiimote.task();
        delay(delayMs);
    }
}

void testWifiRuntimeEnableFromDisabledConfig() {
    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_token";
    cfg.auth.wifiApiToken = "wifi_token";
    cfg.wifi.enabled = false;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;
    cfg.wifi.network = {"media_devices", "Media Devices 3"};

    ESP32Wiimote wiimote(cfg);
    TEST_ASSERT_TRUE(wiimote.init());

    wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    TEST_ASSERT_TRUE(wiimote.isWifiControlEnabled());

    unsigned long startMs = millis();
    ESP32Wiimote::WifiControlState state = wiimote.getWifiControlState();
    while (!state.ready && !state.networkConnectFailed && (millis() - startMs < 20000UL)) {
        wiimote.task();
        delay(50);
        state = wiimote.getWifiControlState();
    }

    TEST_ASSERT_TRUE_MESSAGE(state.ready || state.networkConnectFailed,
                             "Wi-Fi should either connect or fail with diagnostics");
    if (state.ready) {
        TEST_ASSERT_TRUE(state.enabled);
        TEST_ASSERT_TRUE(state.networkConnected);
        TEST_ASSERT_FALSE(state.networkConnectFailed);
        TEST_ASSERT_TRUE(state.staticRoutesRegistered);
        TEST_ASSERT_TRUE(state.apiRoutesRegistered);
        TEST_ASSERT_FALSE(state.websocketRoutesRegistered);
    }
}

void testWifiRuntimeEnableFailsWithoutCredentials() {
    ESP32WiimoteConfig cfg;
    cfg.auth.serialPrivilegedToken = "serial_token";
    cfg.auth.wifiApiToken = "wifi_token";
    cfg.wifi.enabled = false;
    cfg.wifi.deliveryMode = WifiDeliveryMode::RestOnly;

    ESP32Wiimote wiimote(cfg);
    TEST_ASSERT_TRUE(wiimote.init());

    wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    TEST_ASSERT_TRUE(wiimote.isWifiControlEnabled());

    wiimote.task();

    const ESP32Wiimote::WifiControlState state = wiimote.getWifiControlState();
    TEST_ASSERT_FALSE(state.enabled);
    TEST_ASSERT_FALSE(state.ready);
    TEST_ASSERT_TRUE(state.networkConnectAttempted);
    TEST_ASSERT_FALSE(state.networkConnected);
    TEST_ASSERT_TRUE(state.networkConnectFailed);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);

    UNITY_BEGIN();
    RUN_TEST(testWifiRuntimeEnableFromDisabledConfig);
    RUN_TEST(testWifiRuntimeEnableFailsWithoutCredentials);
    UNITY_END();
}

void loop() {}
