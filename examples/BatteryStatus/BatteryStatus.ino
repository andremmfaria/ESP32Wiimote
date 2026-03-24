#include "ESP32Wiimote.h"

#include <Arduino.h>

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this battery-focused example.
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = false;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}

ESP32Wiimote wiimote(makeWiimoteConfig());

static unsigned long lastRequestMs = 0;
static const unsigned long kRequestIntervalMs = 5000;

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("BatteryStatus: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (wiimote.getConfig().wifi.enabled) {
        wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
    }

    Serial.println("Ready. Press 1 + 2 to connect.");
}

void loop() {
    wiimote.task();

    if (!ESP32Wiimote::isConnected()) {
        delay(50);
        return;
    }

    unsigned long now = millis();
    if (now - lastRequestMs >= kRequestIntervalMs) {
        ESP32Wiimote::requestBatteryUpdate();
        delay(100);

        uint8_t batteryRaw = ESP32Wiimote::getBatteryLevel();
        float batteryPercent = (static_cast<float>(batteryRaw) / 255.0F) * 100.0F;
        Serial.printf("battery: %3u/255 (%.1f%%)\n", batteryRaw, batteryPercent);

        lastRequestMs = now;
    }

    delay(20);
}
