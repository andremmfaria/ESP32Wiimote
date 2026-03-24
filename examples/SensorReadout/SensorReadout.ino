#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32WiimoteConfig config = {
    5,
    32,
    32,
    180000,
};
ESP32Wiimote wiimote(config);

// Current runtime config model (auth + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this sensor-focused example.
static const bool kEnableWifiControl = false;
static const char *kApiUsername = "admin";
static const char *kApiPassword = "password";
static const char *kBearerToken = "esp32wiimote_bearer_token_v1";
static const char *kWifiSsid = "YOUR_WIFI_SSID";
static const char *kWifiNetworkPassword = "YOUR_WIFI_PASSWORD";

void setup() {
    Serial.begin(115200);
    delay(200);

    WiimoteConfig runtimeConfig = {
        kEnableWifiControl,
        {kApiUsername, kApiPassword, kBearerToken},
        {kWifiSsid, kWifiNetworkPassword},
    };
    wiimote.configure(runtimeConfig);

    Serial.println("SensorReadout: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (kEnableWifiControl) {
        wiimote.enableWifiControl(true, WifiDeliveryMode::RestOnly);
    }

    Serial.println("Ready. Press 1 + 2 and move Wiimote/Nunchuk.");
}

void loop() {
    wiimote.task();

    while (wiimote.available() > 0) {
        AccelState accel = wiimote.getAccelState();
        NunchukState nunchuk = wiimote.getNunchukState();

        Serial.printf("wiimote accel: %3u/%3u/%3u", accel.xAxis, accel.yAxis, accel.zAxis);
        Serial.printf(" | nunchuk accel: %3u/%3u/%3u", nunchuk.xAxis, nunchuk.yAxis, nunchuk.zAxis);
        Serial.printf(" | nunchuk stick: %3u/%3u\n", nunchuk.xStick, nunchuk.yStick);
    }

    delay(10);
}
