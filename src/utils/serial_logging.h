// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef WIIMOTE_VERBOSE_H
#define WIIMOTE_VERBOSE_H

#include "Arduino.h"

/**
 * Logging Level Configuration
 * 
 * Set WIIMOTE_VERBOSE to control logging verbosity:
 *   0 = Errors only (always shown)
 *   1 = Errors + Warnings
 *   2 = Errors + Warnings + Info
 *   3 = Errors + Warnings + Info + Debug (full verbose)
 */
#define WIIMOTE_VERBOSE 2

// Log level thresholds
#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_DEBUG   3

/**
 * ERROR: Critical errors that always need attention
 * Always shown regardless of verbose setting
 */
#define LOG_ERROR(...) Serial.printf("[ERROR] " __VA_ARGS__)

/**
 * WARNING: Important issues that should be noted
 * Shown when WIIMOTE_VERBOSE >= 1
 */
#if WIIMOTE_VERBOSE >= LOG_LEVEL_WARNING
#define LOG_WARN(...) Serial.printf("[WARN] " __VA_ARGS__)
#else
#define LOG_WARN(...) do {} while(0)
#endif

/**
 * INFO: General informational messages
 * Shown when WIIMOTE_VERBOSE >= 2
 */
#if WIIMOTE_VERBOSE >= LOG_LEVEL_INFO
#define LOG_INFO(...) Serial.printf("[INFO] " __VA_ARGS__)
#else
#define LOG_INFO(...) do {} while(0)
#endif

/**
 * DEBUG: Detailed debug information
 * Shown when WIIMOTE_VERBOSE >= 3
 */
#if WIIMOTE_VERBOSE >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) Serial.printf("[DEBUG] " __VA_ARGS__)
#else
#define LOG_DEBUG(...) do {} while(0)
#endif

#endif // WIIMOTE_VERBOSE_H
