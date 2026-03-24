#include "ESP32Wiimote.h"

#include <Arduino.h>

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this button-focused example.
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = false;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}

ESP32Wiimote wiimote(makeWiimoteConfig());

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("ButtonInput: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (wiimote.getConfig().wifi.enabled) {
        wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
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
