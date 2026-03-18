// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md
//
// The short of it is...
//   You are free to:
//     Share — copy and redistribute the material in any medium or format
//     Adapt — remix, transform, and build upon the material
//   Under the following terms:
//     NonCommercial — You may not use the material for commercial purposes.

#ifndef ESP32_WIIMOTE_TINYWIIMOTE_H
#define ESP32_WIIMOTE_TINYWIIMOTE_H

#include "tinywiimote/protocol/wiimote_reports.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct TwHciInterface {
    void (*hciSendPacket)(uint8_t *data, size_t len);
};

void tinyWiimoteInit(struct TwHciInterface hciInterface);
int tinyWiimoteAvailable();
TinyWiimoteData tinyWiimoteRead();

void tinyWiimoteResetDevice();
bool tinyWiimoteDeviceIsInited();
bool tinyWiimoteIsConnected();
uint8_t tinyWiimoteGetBatteryLevel();
void tinyWiimoteRequestBatteryUpdate();

void tinyWiimoteReqAccelerometer(bool use);
void tinyWiimoteSetFastReconnectTtlMs(uint32_t ttlMs);
void tinyWiimoteSetScanEnabled(bool enabled);
bool tinyWiimoteStartDiscovery();
bool tinyWiimoteStopDiscovery();

void handleHciData(uint8_t *data, size_t len);

char *format2Hex(uint8_t *data, uint16_t len);

#endif  // ESP32_WIIMOTE_TINYWIIMOTE_H
