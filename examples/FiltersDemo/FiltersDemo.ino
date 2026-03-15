#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

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

    if (kIgnoreAccel) {
        wiimote.addFilter(FilterAction::Ignore, FilterAccel);
        Serial.println("Filter enabled: FilterAccel");
    }
    if (kIgnoreNunchukStick) {
        wiimote.addFilter(FilterAction::Ignore, FilterNunchukStick);
        Serial.println("Filter enabled: FilterNunchukStick");
    }
    if (kIgnoreButtons) {
        wiimote.addFilter(FilterAction::Ignore, FilterButton);
        Serial.println("Filter enabled: FilterButton");
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
