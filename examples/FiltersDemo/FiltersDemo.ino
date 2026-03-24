#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this filter-focused example.
static const bool kEnableWifiControl = false;
static const char *kSerialPrivilegedToken = "esp32wiimote_serial_token_v1";
static const char *kWifiApiToken = "esp32wiimote_wifi_api_token_v1";
static const char *kWifiSsid = "YOUR_WIFI_SSID";
static const char *kWifiNetworkPassword = "YOUR_WIFI_PASSWORD";

// Toggle these to reduce update volume.
static const bool kIgnoreAccel = true;
static const bool kIgnoreNunchukStick = false;
static const bool kIgnoreButtons = false;

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

    Serial.println("FiltersDemo: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (kEnableWifiControl) {
        wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
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
