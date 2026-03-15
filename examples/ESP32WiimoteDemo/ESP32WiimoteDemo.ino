#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32Wiimote wiimote;

// Demo options
static const bool kEnableVerboseInputLog = true;
static const bool kIgnoreAccel = false;
static const bool kIgnoreNunchukStick = false;
static const bool kIgnoreButtons = false;

// Runtime state
static bool wasConnected = false;
static unsigned long lastStatsMs = 0;
static unsigned long lastBatteryMs = 0;
static int numLoopRuns = 0;
static int numInputUpdates = 0;

static const unsigned long kStatsIntervalMs = 1000;
static const unsigned long kBatteryIntervalMs = 3000;

static void printButtonLine(ButtonState button) {
    char ca = ((button & ButtonA) != 0) ? 'A' : '.';
    char cb = ((button & ButtonB) != 0) ? 'B' : '.';
    char cc = ((button & ButtonC) != 0) ? 'C' : '.';
    char cz = ((button & ButtonZ) != 0) ? 'Z' : '.';
    char c1 = ((button & ButtonOne) != 0) ? '1' : '.';
    char c2 = ((button & ButtonTwo) != 0) ? '2' : '.';
    char cminus = ((button & ButtonMinus) != 0) ? '-' : '.';
    char cplus = ((button & ButtonPlus) != 0) ? '+' : '.';
    char chome = ((button & ButtonHome) != 0) ? 'H' : '.';
    char cleft = ((button & ButtonLeft) != 0) ? '<' : '.';
    char cright = ((button & ButtonRight) != 0) ? '>' : '.';
    char cup = ((button & ButtonUp) != 0) ? '^' : '.';
    char cdown = ((button & ButtonDown) != 0) ? 'v' : '.';

    Serial.printf("buttons: %05x = ", (int)button);
    Serial.print(ca);
    Serial.print(cb);
    Serial.print(cc);
    Serial.print(cz);
    Serial.print(c1);
    Serial.print(c2);
    Serial.print(cminus);
    Serial.print(chome);
    Serial.print(cplus);
    Serial.print(cleft);
    Serial.print(cright);
    Serial.print(cup);
    Serial.print(cdown);
}

static void printBatteryLine() {
    uint8_t battery = ESP32Wiimote::getBatteryLevel();
    float batteryPercent = (static_cast<float>(battery) / 255.0F) * 100.0F;
    Serial.printf("battery: %3u/255 (%.1f%%)\n", battery, batteryPercent);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("\n\n===== ESP32Wiimote Demo =====");
    Serial.println("Features: connection, battery, buttons, accel, nunchuk, filters");
    Serial.println("Initializing Bluetooth controller...");

    if (!wiimote.init()) {
        Serial.println("FATAL: Bluetooth initialization failed! Halting.");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("Bluetooth initialized successfully!");

    if (kIgnoreAccel) {
        wiimote.addFilter(FilterAction::Ignore, FilterAccel);
    }
    if (kIgnoreNunchukStick) {
        wiimote.addFilter(FilterAction::Ignore, FilterNunchukStick);
    }
    if (kIgnoreButtons) {
        wiimote.addFilter(FilterAction::Ignore, FilterButton);
    }

    lastStatsMs = millis();
    lastBatteryMs = millis();

    Serial.println("Ready! Press 1 + 2 on the Wiimote to connect.");
}

void loop() {
    wiimote.task();
    numLoopRuns++;

    bool isConnected = ESP32Wiimote::isConnected();
    if (isConnected != wasConnected) {
        Serial.printf("connection: %s\n", isConnected ? "CONNECTED" : "DISCONNECTED");
        wasConnected = isConnected;
    }

    unsigned long now = millis();
    if (isConnected && (now - lastBatteryMs >= kBatteryIntervalMs)) {
        printBatteryLine();
        lastBatteryMs = now;
    }

    while (wiimote.available() > 0) {
        ButtonState button = wiimote.getButtonState();
        AccelState accel = wiimote.getAccelState();
        NunchukState nunchuk = wiimote.getNunchukState();
        numInputUpdates++;

        if (kEnableVerboseInputLog) {
            printButtonLine(button);
            Serial.printf(", wiimote.axis: %3u/%3u/%3u", accel.xAxis, accel.yAxis, accel.zAxis);
            Serial.printf(", nunchuk.axis: %3u/%3u/%3u", nunchuk.xAxis, nunchuk.yAxis,
                          nunchuk.zAxis);
            Serial.printf(", nunchuk.stick: %3u/%3u\n", nunchuk.xStick, nunchuk.yStick);
        }
    }

    if (now - lastStatsMs >= kStatsIntervalMs) {
        Serial.printf("stats: loops/s=%d, updates/s=%d, connected=%d\n", numLoopRuns,
                      numInputUpdates, (int)isConnected);
        numLoopRuns = 0;
        numInputUpdates = 0;
        lastStatsMs += kStatsIntervalMs;
        if ((now - lastStatsMs) >= kStatsIntervalMs) {
            lastStatsMs = now;
        }
    }

    delay(10);
}
