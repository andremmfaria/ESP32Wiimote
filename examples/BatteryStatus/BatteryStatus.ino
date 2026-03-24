#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

// Current runtime config model (auth + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this battery-focused example.
static const bool kEnableWifiControl = false;
static const char *kApiUsername = "admin";
static const char *kApiPassword = "password";
static const char *kBearerToken = "esp32wiimote_bearer_token_v1";
static const char *kWifiSsid = "YOUR_WIFI_SSID";
static const char *kWifiNetworkPassword = "YOUR_WIFI_PASSWORD";

static unsigned long lastRequestMs = 0;
static const unsigned long kRequestIntervalMs = 5000;

void setup() {
    Serial.begin(115200);
    delay(200);

    WiimoteConfig runtimeConfig = {
        kEnableWifiControl,
        {kApiUsername, kApiPassword, kBearerToken},
        {kWifiSsid, kWifiNetworkPassword},
    };
    wiimote.configure(runtimeConfig);

    Serial.println("BatteryStatus: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (kEnableWifiControl) {
        wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
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
