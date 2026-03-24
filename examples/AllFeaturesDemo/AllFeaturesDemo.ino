#include "ESP32Wiimote.h"

#include <Arduino.h>

ESP32WiimoteConfig config = {
    1,
    32,
    32,
    180000,
};
ESP32Wiimote wiimote(config);

// Demo options
static const bool kEnableVerboseInputLog = true;
static const bool kIgnoreAccel = false;
static const bool kIgnoreNunchukStick = false;
static const bool kIgnoreButtons = false;

// Set true to demo Wi-Fi control lifecycle and REST/WebSocket delivery mode.
static const bool kEnableWifiControl = false;
static const WifiDeliveryMode kWifiDeliveryMode = WifiDeliveryMode::RestAndWebSocket;
static const char *kSerialPrivilegedToken = "esp32wiimote_serial_token_v1";
static const char *kWifiApiToken = "esp32wiimote_wifi_api_token_v1";
static const char *kWifiSsid = "YOUR_WIFI_SSID";
static const char *kWifiNetworkPassword = "YOUR_WIFI_PASSWORD";

// Runtime state
static bool wasConnected = false;
static unsigned long lastStatsMs = 0;
static unsigned long lastBatteryMs = 0;
static unsigned long lastWifiStateMs = 0;
static int numLoopRuns = 0;
static int numInputUpdates = 0;

static const unsigned long kStatsIntervalMs = 1000;
static const unsigned long kBatteryIntervalMs = 3000;
static const unsigned long kWifiStateIntervalMs = 2000;

static void printButtonLine(ButtonState button) {
    char ca = buttonStateHas(button, kButtonA) ? 'A' : '.';
    char cb = buttonStateHas(button, kButtonB) ? 'B' : '.';
    char cc = buttonStateHas(button, kButtonC) ? 'C' : '.';
    char cz = buttonStateHas(button, kButtonZ) ? 'Z' : '.';
    char c1 = buttonStateHas(button, kButtonOne) ? '1' : '.';
    char c2 = buttonStateHas(button, kButtonTwo) ? '2' : '.';
    char cminus = buttonStateHas(button, kButtonMinus) ? '-' : '.';
    char cplus = buttonStateHas(button, kButtonPlus) ? '+' : '.';
    char chome = buttonStateHas(button, kButtonHome) ? 'H' : '.';
    char cleft = buttonStateHas(button, kButtonLeft) ? '<' : '.';
    char cright = buttonStateHas(button, kButtonRight) ? '>' : '.';
    char cup = buttonStateHas(button, kButtonUp) ? '^' : '.';
    char cdown = buttonStateHas(button, kButtonDown) ? 'v' : '.';

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

static void printWifiStateLine(const ESP32Wiimote::WifiControlState &state) {
    const char *deliveryMode =
        state.deliveryMode == WifiDeliveryMode::RestAndWebSocket ? "RestAndWebSocket" : "RestOnly";
    Serial.printf("wifi: ready=%d net=%d failed=%d mode=%s static=%d api=%d ws=%d\n",
                  (int)state.ready, (int)state.networkConnected, (int)state.networkConnectFailed,
                  deliveryMode, (int)state.staticRoutesRegistered, (int)state.apiRoutesRegistered,
                  (int)state.websocketRoutesRegistered);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println("\n\n===== ESP32Wiimote All Features Demo =====");
    Serial.println("Features: connection, battery, buttons, accel, nunchuk, filters, Wi-Fi state");
    Serial.println("Initializing Bluetooth controller...");

    WiimoteConfig runtimeConfig = {
        kEnableWifiControl,
        kSerialPrivilegedToken,
        kWifiApiToken,
        {kWifiSsid, kWifiNetworkPassword},
    };
    wiimote.configure(runtimeConfig);

    if (!wiimote.init()) {
        Serial.println("FATAL: Bluetooth initialization failed! Halting.");
        while (true) {
            delay(1000);
        }
    }

    if (kEnableWifiControl) {
        wiimote.enableWifiControl(true, kWifiDeliveryMode);
        Serial.println("Wi-Fi control enabled (async startup in task loop)");
    }

    Serial.println("Bluetooth initialized successfully!");

    if (kIgnoreAccel) {
        wiimote.addFilter(FilterAction::Ignore, kFilterAccel);
    }
    if (kIgnoreNunchukStick) {
        wiimote.addFilter(FilterAction::Ignore, kFilterNunchukStick);
    }
    if (kIgnoreButtons) {
        wiimote.addFilter(FilterAction::Ignore, kFilterButton);
    }

    lastStatsMs = millis();
    lastBatteryMs = millis();
    lastWifiStateMs = millis();

    Serial.println("Ready! Press 1 + 2 on the Wiimote to connect.");
}

void loop() {
    wiimote.task();
    numLoopRuns++;

    unsigned long now = millis();

    bool isConnected = ESP32Wiimote::isConnected();
    if (isConnected != wasConnected) {
        Serial.printf("connection: %s\n", isConnected ? "CONNECTED" : "DISCONNECTED");
        wasConnected = isConnected;
    }

    if (isConnected && (now - lastBatteryMs >= kBatteryIntervalMs)) {
        printBatteryLine();
        lastBatteryMs = now;
    }

    if (kEnableWifiControl && (now - lastWifiStateMs >= kWifiStateIntervalMs)) {
        printWifiStateLine(wiimote.getWifiControlState());
        lastWifiStateMs = now;
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
