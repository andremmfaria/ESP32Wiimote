#include "ESP32Wiimote.h"

#include <Arduino.h>

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this filter-focused example.
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.auth.serialPrivilegedToken = "esp32wiimote_serial_token_v1";
    config.auth.wifiApiToken = "esp32wiimote_wifi_api_token_v1";
    config.wifi.enabled = false;
    config.wifi.network = {"YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD"};
    return config;
}

ESP32Wiimote wiimote(makeWiimoteConfig());

// Toggle these to reduce update volume.
static const bool kIgnoreAccel = true;
static const bool kIgnoreNunchukStick = false;
static const bool kIgnoreButtons = false;

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("FiltersDemo: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (wiimote.getConfig().wifi.enabled) {
        wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
    }

    if (kIgnoreAccel) {
        wiimote.addFilter(FilterAction::Ignore, kFilterAccel);
        Serial.println("Filter enabled: kFilterAccel");
    }
    if (kIgnoreNunchukStick) {
        wiimote.addFilter(FilterAction::Ignore, kFilterNunchukStick);
        Serial.println("Filter enabled: kFilterNunchukStick");
    }
    if (kIgnoreButtons) {
        wiimote.addFilter(FilterAction::Ignore, kFilterButton);
        Serial.println("Filter enabled: kFilterButton");
    }

    Serial.println("Ready. Press 1 + 2.");
}

void loop() {
    wiimote.task();

    while (wiimote.available() > 0) {
        ButtonState buttons = wiimote.getButtonState();
        AccelState accel = wiimote.getAccelState();

        Serial.printf("update: buttons=0x%05x accel=%3u/%3u/%3u\n", (int)buttons, accel.xAxis,
                      accel.yAxis, accel.zAxis);
    }

    delay(10);
}
