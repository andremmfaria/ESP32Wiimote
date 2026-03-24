#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

// Current runtime config model (auth + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this basic example.
static const bool kEnableWifiControl = false;
static const char *kApiUsername = "admin";
static const char *kApiPassword = "password";
static const char *kBearerToken = "esp32wiimote_bearer_token_v1";
static const char *kWifiSsid = "YOUR_WIFI_SSID";
static const char *kWifiNetworkPassword = "YOUR_WIFI_PASSWORD";

static bool wasConnected = false;

void setup() {
    Serial.begin(115200);
    delay(200);

    WiimoteConfig runtimeConfig = {
        kEnableWifiControl,
        {kApiUsername, kApiPassword, kBearerToken},
        {kWifiSsid, kWifiNetworkPassword},
    };
    wiimote.configure(runtimeConfig);

    Serial.println("BasicConnection: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (kEnableWifiControl) {
        wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
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
