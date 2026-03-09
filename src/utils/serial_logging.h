// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef WIIMOTE_VERBOSE_H
#define WIIMOTE_VERBOSE_H

#include "Arduino.h"

// Set to 1 to enable verbose debug logging, 0 to disable
#define WIIMOTE_VERBOSE 0

#if WIIMOTE_VERBOSE
#define VERBOSE_PRINT(...) Serial.printf(__VA_ARGS__)
#define VERBOSE_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define VERBOSE_PRINT(...) do {} while(0)
#define VERBOSE_PRINTLN(...) do {} while(0)
#endif

// UNVERBOSE_PRINT is always enabled for critical messages and errors
#define UNVERBOSE_PRINT(...) Serial.printf(__VA_ARGS__)

#endif // WIIMOTE_VERBOSE_H
