// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "serial_logging.h"

#include <Arduino.h>

#include <stdarg.h>
#include <stdio.h>

static uint8_t sanitizeLogLevel(uint8_t level) {
    if (level > static_cast<uint8_t>(WiimoteLogLevel::Debug)) {
        return static_cast<uint8_t>(WiimoteLogLevel::Debug);
    }

    return level;
}

static uint8_t gWiimoteLogLevel = sanitizeLogLevel(WIIMOTE_VERBOSE);

uint8_t wiimoteGetLogLevel() {
    return gWiimoteLogLevel;
}

void wiimoteSetLogLevel(uint8_t level) {
    gWiimoteLogLevel = sanitizeLogLevel(level);
}

void wiimoteVLogPrint(uint8_t level, const char *prefix, const char *format, va_list args) {
    if (format == nullptr || prefix == nullptr || level > gWiimoteLogLevel) {
        return;
    }

    Serial.print(prefix);

    char buffer[256];
    const int kWritten = vsnprintf(buffer, sizeof(buffer), format, args);

    if (kWritten <= 0) {
        return;
    }

    Serial.printf("%s", buffer);
}

void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteVLogPrint(level, prefix, format, args);
    va_end(args);
}

void wiimoteLogError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteVLogPrint(wiimoteLogLevelValue(WiimoteLogLevel::Error), "[ERROR] ", format, args);
    va_end(args);
}

void wiimoteLogWarn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteVLogPrint(wiimoteLogLevelValue(WiimoteLogLevel::Warning), "[WARN] ", format, args);
    va_end(args);
}

void wiimoteLogInfo(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteVLogPrint(wiimoteLogLevelValue(WiimoteLogLevel::Info), "[INFO] ", format, args);
    va_end(args);
}

void wiimoteLogDebug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    wiimoteVLogPrint(wiimoteLogLevelValue(WiimoteLogLevel::Debug), "[DEBUG] ", format, args);
    va_end(args);
}
