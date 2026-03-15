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
    if (level > (uint8_t)WiimoteLogDebug) {
        return (uint8_t)WiimoteLogDebug;
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

void wiimoteLogVPrint(uint8_t level, const char *prefix, const char *format, va_list args) {
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
    wiimoteLogVPrint(level, prefix, format, args);
    va_end(args);
}
