#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

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

    Serial.println("Ready. Press 1 + 2, then press buttons.");
}

void loop() {
    wiimote.task();

    while (wiimote.available() > 0) {
        ButtonState buttons = wiimote.getButtonState();
        Serial.printf("buttons mask: 0x%05x\n", (int)buttons);

        if ((buttons & ButtonA) != 0) {
            Serial.println("A pressed");
        }
    }

    delay(10);
}
