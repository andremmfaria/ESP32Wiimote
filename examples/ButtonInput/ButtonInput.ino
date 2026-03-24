#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this button-focused example.
static const bool kEnableWifiControl = false;
static const char *kSerialPrivilegedToken = "esp32wiimote_serial_token_v1";
static const char *kWifiApiToken = "esp32wiimote_wifi_api_token_v1";
static const char *kWifiSsid = "YOUR_WIFI_SSID";
static const char *kWifiNetworkPassword = "YOUR_WIFI_PASSWORD";

void setup() {
    Serial.begin(115200);
    delay(200);

    WiimoteConfig runtimeConfig = {
        kEnableWifiControl,
        kSerialPrivilegedToken,
        kWifiApiToken,
        {kWifiSsid, kWifiNetworkPassword},
    };
    wiimote.configure(runtimeConfig);

    Serial.println("ButtonInput: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (kEnableWifiControl) {
        wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    }

    Serial.println("Ready. Press 1 + 2, then press buttons.");
}

void loop() {
    wiimote.task();

    while (wiimote.available() > 0) {
        ButtonState buttons = wiimote.getButtonState();
        Serial.printf("buttons mask: 0x%05x\n", (int)buttons);

        if (buttonStateHas(buttons, kButtonA)) {
            Serial.println("A pressed");
        }
    }

    delay(10);
}
