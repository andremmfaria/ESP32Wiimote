// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "serial_logging.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef WIIMOTE_VERBOSE
#define WIIMOTE_VERBOSE WIIMOTE_LOG_WARNING
#endif

static uint8_t sanitizeLogLevel(uint8_t level) {
    if (level > WIIMOTE_LOG_DEBUG) {
        return WIIMOTE_LOG_DEBUG;
    }

    return level;
}

static uint8_t g_wiimoteLogLevel = sanitizeLogLevel(WIIMOTE_VERBOSE);

uint8_t wiimoteGetLogLevel() {
    return g_wiimoteLogLevel;
}

void wiimoteSetLogLevel(uint8_t level) {
    g_wiimoteLogLevel = sanitizeLogLevel(level);
}

void wiimoteLogPrint(uint8_t level, const char *prefix, const char *format, ...) {
    if (format == nullptr || prefix == nullptr || level > g_wiimoteLogLevel) {
        return;
    }

    Serial.print(prefix);

    char buffer[256];
    va_list args;
    va_start(args, format);
    const int written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (written <= 0) {
        return;
    }

    Serial.printf("%s", buffer);
}
