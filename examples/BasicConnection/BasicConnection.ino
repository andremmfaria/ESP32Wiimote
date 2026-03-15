#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

static bool wasConnected = false;

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("BasicConnection: initializing...");
    if (!wiimote.init()) {
        Serial.println("Init failed. Halting.");
        while (true) {
            delay(1000);
        }
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
