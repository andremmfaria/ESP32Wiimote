#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32WiimoteConfig config = {
    5,
    32,
    32,
    180000,
};
ESP32Wiimote wiimote(config);

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
