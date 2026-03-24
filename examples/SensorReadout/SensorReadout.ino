#include "ESP32Wiimote.h"

#include <Arduino.h>

// Current runtime config model (tokens + optional Wi-Fi network credentials).
// Keep Wi-Fi control disabled in this sensor-focused example.
static ESP32WiimoteConfig makeWiimoteConfig() {
    ESP32WiimoteConfig config;
    config.nunchukStickThreshold = 5;
    config.txQueueSize = 32;
    config.rxQueueSize = 32;
    config.fastReconnectTtlMs = 180000;
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

    Serial.println("SensorReadout: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (wiimote.getConfig().wifi.enabled) {
        wiimote.enableWifiControl(true, wiimote.getConfig().wifi.deliveryMode);
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
