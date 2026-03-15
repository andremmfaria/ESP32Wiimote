// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_UTILS_SERIAL_LOGGING_H
#define ESP32_WIIMOTE_UTILS_SERIAL_LOGGING_H

#include <stdarg.h>
#include <stdint.h>

enum WiimoteLogLevel : uint8_t {
    WiimoteLogError = 0,
    WiimoteLogWarning = 1,
    WiimoteLogInfo = 2,
    WiimoteLogDebug = 3,
};

#define WIIMOTE_LOG_ERROR ((uint8_t)WiimoteLogError)
#define WIIMOTE_LOG_WARNING ((uint8_t)WiimoteLogWarning)
#define WIIMOTE_LOG_INFO ((uint8_t)WiimoteLogInfo)
#define WIIMOTE_LOG_DEBUG ((uint8_t)WiimoteLogDebug)

uint8_t wiimoteGetLogLevel();
void wiimoteSetLogLevel(uint8_t level);
void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...);

#define LOG_ERROR(...) wiimoteLogPrint(WIIMOTE_LOG_ERROR, "[ERROR] ", __VA_ARGS__)
#define LOG_WARN(...) wiimoteLogPrint(WIIMOTE_LOG_WARNING, "[WARN] ", __VA_ARGS__)
#define LOG_INFO(...) wiimoteLogPrint(WIIMOTE_LOG_INFO, "[INFO] ", __VA_ARGS__)
#define LOG_DEBUG(...) wiimoteLogPrint(WIIMOTE_LOG_DEBUG, "[DEBUG] ", __VA_ARGS__)

#endif  // ESP32_WIIMOTE_UTILS_SERIAL_LOGGING_H
