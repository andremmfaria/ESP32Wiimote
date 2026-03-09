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

#ifndef _TINY_WIIMOTE_H_
#define _TINY_WIIMOTE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tinywiimote/protocol/wiimote_reports.h"

struct TwHciInterface {
    void (*hci_send_packet)(uint8_t *data, size_t len);
};

void TinyWiimoteInit(struct TwHciInterface hciInterface);
int TinyWiimoteAvailable(void);
TinyWiimoteData TinyWiimoteRead(void);

void TinyWiimoteResetDevice(void);
bool TinyWiimoteDeviceIsInited(void);
bool TinyWiimoteIsConnected(void);
uint8_t TinyWiimoteGetBatteryLevel(void);
void TinyWiimoteRequestBatteryUpdate(void);

void TinyWiimoteReqAccelerometer(bool use);

void handleHciData(uint8_t* data, size_t len);

char* format2Hex(uint8_t* data, uint16_t len);

#endif // _TINY_WIIMOTE_H_
