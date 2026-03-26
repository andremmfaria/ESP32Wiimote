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

enum class WiimoteLogLevel : uint8_t {
    Error = 0,
    Warning = 1,
    Info = 2,
    Debug = 3,
};

constexpr uint8_t wiimoteLogLevelValue(const WiimoteLogLevel level) {
    return static_cast<uint8_t>(level);
}

constexpr uint8_t kWiimoteLogError = wiimoteLogLevelValue(WiimoteLogLevel::Error);
constexpr uint8_t kWiimoteLogWarning = wiimoteLogLevelValue(WiimoteLogLevel::Warning);
constexpr uint8_t kWiimoteLogInfo = wiimoteLogLevelValue(WiimoteLogLevel::Info);
constexpr uint8_t kWiimoteLogDebug = wiimoteLogLevelValue(WiimoteLogLevel::Debug);

#ifndef WIIMOTE_VERBOSE
#define WIIMOTE_VERBOSE 2
#endif

uint8_t wiimoteGetLogLevel();
void wiimoteSetLogLevel(uint8_t level);
void wiimoteVLogPrint(uint8_t level, const char *prefix, const char *format, va_list args);

void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...);
void wiimoteLogError(const char *format, ...);
void wiimoteLogWarn(const char *format, ...);
void wiimoteLogInfo(const char *format, ...);
void wiimoteLogDebug(const char *format, ...);

#endif  // ESP32_WIIMOTE_UTILS_SERIAL_LOGGING_H
