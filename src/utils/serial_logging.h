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

static constexpr uint8_t kWiimoteLogError = static_cast<uint8_t>(WiimoteLogLevel::Error);
static constexpr uint8_t kWiimoteLogWarning = static_cast<uint8_t>(WiimoteLogLevel::Warning);
static constexpr uint8_t kWiimoteLogInfo = static_cast<uint8_t>(WiimoteLogLevel::Info);
static constexpr uint8_t kWiimoteLogDebug = static_cast<uint8_t>(WiimoteLogLevel::Debug);

#ifndef WIIMOTE_VERBOSE
#define WIIMOTE_VERBOSE 2
#endif

uint8_t wiimoteGetLogLevel();
void wiimoteSetLogLevel(uint8_t level);
void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...);

#define wiimoteLogError(...) wiimoteLogPrint(kWiimoteLogError, "[ERROR] ", __VA_ARGS__)
#define wiimoteLogWarn(...) wiimoteLogPrint(kWiimoteLogWarning, "[WARN] ", __VA_ARGS__)
#define wiimoteLogInfo(...) wiimoteLogPrint(kWiimoteLogInfo, "[INFO] ", __VA_ARGS__)
#define wiimoteLogDebug(...) wiimoteLogPrint(kWiimoteLogDebug, "[DEBUG] ", __VA_ARGS__)

#define LOG_ERROR(...) wiimoteLogError(__VA_ARGS__)
#define LOG_WARN(...) wiimoteLogWarn(__VA_ARGS__)
#define LOG_INFO(...) wiimoteLogInfo(__VA_ARGS__)
#define LOG_DEBUG(...) wiimoteLogDebug(__VA_ARGS__)

#endif  // ESP32_WIIMOTE_UTILS_SERIAL_LOGGING_H
