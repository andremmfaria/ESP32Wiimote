#include "ESP32Wiimote.h"

#include <Arduino.h>

static unsigned long gLastWifiPrintMs = 0;
static const unsigned long kWifiPrintIntervalMs = 1000;

static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = true;
    config.wifi.deliveryMode = WifiDeliveryMode::RestAndWebSocket;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}

ESP32Wiimote wiimote(makeWiimoteConfig());

static const char *deliveryModeToString(WifiDeliveryMode mode) {
    if (mode == WifiDeliveryMode::RestAndWebSocket) {
        return "RestAndWebSocket";
    }
    return "RestOnly";
}

static void printWifiState(const ESP32Wiimote::WifiControlState &state) {
    Serial.printf(
        "wifi: enabled=%d init=%d ready=%d netCfg=%d attempted=%d connected=%d failed=%d mode=%s\n",
        (int)state.enabled, (int)state.initializing, (int)state.ready,
        (int)state.networkCredentialsConfigured, (int)state.networkConnectAttempted,
        (int)state.networkConnected, (int)state.networkConnectFailed,
        deliveryModeToString(state.deliveryMode));
    Serial.printf("      stages: wifi=%d fs=%d static=%d api=%d ws=%d\n",
                  (int)state.wifiLayerStarted, (int)state.littleFsMounted,
                  (int)state.staticRoutesRegistered, (int)state.apiRoutesRegistered,
                  (int)state.websocketRoutesRegistered);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("WifiControlLifecycle: initializing...");

    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);

    Serial.println("Wi-Fi control enabled.");
    Serial.println("REST snapshots: GET /api/wiimote/status and /api/wiimote/config");
    Serial.println(
        "WebSocket streams (RestAndWebSocket): /api/wiimote/input/events and "
        "/api/wiimote/status/events");
    Serial.println("Press 1 + 2 on Wiimote for input traffic.");
}

void loop() {
    wiimote.task();

    unsigned long now = millis();
    if (now - gLastWifiPrintMs >= kWifiPrintIntervalMs) {
        printWifiState(wiimote.getWifiControlState());
        gLastWifiPrintMs = now;
    }

    if (wiimote.available() > 0) {
        ButtonState buttons = wiimote.getButtonState();
        Serial.printf("input: buttons=0x%05x\n", (int)buttons);
    }

    delay(10);
}
