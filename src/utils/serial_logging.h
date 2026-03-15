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

#ifndef WIIMOTE_VERBOSE
#define WIIMOTE_VERBOSE 2
#endif

uint8_t wiimoteGetLogLevel();
void wiimoteSetLogLevel(uint8_t level);
void wiimoteLogVPrint(uint8_t level, const char *prefix, const char *format, va_list args);
void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...);

inline void wiimoteLogError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteLogVPrint(WIIMOTE_LOG_ERROR, "[ERROR] ", format, args);
    va_end(args);
}

inline void wiimoteLogWarn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteLogVPrint(WIIMOTE_LOG_WARNING, "[WARN] ", format, args);
    va_end(args);
}

inline void wiimoteLogInfo(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteLogVPrint(WIIMOTE_LOG_INFO, "[INFO] ", format, args);
    va_end(args);
}

inline void wiimoteLogDebug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteLogVPrint(WIIMOTE_LOG_DEBUG, "[DEBUG] ", format, args);
    va_end(args);
}

#define LOG_ERROR(...) wiimoteLogError(__VA_ARGS__)
#define LOG_WARN(...) wiimoteLogWarn(__VA_ARGS__)
#define LOG_INFO(...) wiimoteLogInfo(__VA_ARGS__)
#define LOG_DEBUG(...) wiimoteLogDebug(__VA_ARGS__)

#endif  // ESP32_WIIMOTE_UTILS_SERIAL_LOGGING_H
