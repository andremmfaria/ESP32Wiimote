// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef WIIMOTE_VERBOSE_H
#define WIIMOTE_VERBOSE_H

#include "Arduino.h"

#include <stdarg.h>

enum WiimoteLogLevel : uint8_t {
    WIIMOTE_LOG_ERROR = 0,
    WIIMOTE_LOG_WARNING = 1,
    WIIMOTE_LOG_INFO = 2,
    WIIMOTE_LOG_DEBUG = 3,
};

uint8_t wiimoteGetLogLevel();
void wiimoteSetLogLevel(uint8_t level);
void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...);

#define LOG_ERROR(...) wiimoteLogPrint(WIIMOTE_LOG_ERROR, "[ERROR] ", __VA_ARGS__)
#define LOG_WARN(...) wiimoteLogPrint(WIIMOTE_LOG_WARNING, "[WARN] ", __VA_ARGS__)
#define LOG_INFO(...) wiimoteLogPrint(WIIMOTE_LOG_INFO, "[INFO] ", __VA_ARGS__)
#define LOG_DEBUG(...) wiimoteLogPrint(WIIMOTE_LOG_DEBUG, "[DEBUG] ", __VA_ARGS__)

#endif  // WIIMOTE_VERBOSE_H
