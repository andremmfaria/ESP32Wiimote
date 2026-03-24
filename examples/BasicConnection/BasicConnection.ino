#include "ESP32Wiimote.h"

#include <Arduino.h>

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this basic example.
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = false;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}

ESP32Wiimote wiimote(makeWiimoteConfig());

static bool wasConnected = false;

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("BasicConnection: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (wiimote.getConfig().wifi.enabled) {
        wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
    }

    Serial.println("Ready. Press 1 + 2 on the Wiimote.");
}

void loop() {
    wiimote.task();

    bool connected = ESP32Wiimote::isConnected();
    if (connected != wasConnected) {
        Serial.println(connected ? "CONNECTED" : "DISCONNECTED");
        wasConnected = connected;
    }

    delay(20);
}
