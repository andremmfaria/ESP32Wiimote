// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_HCI_UTILS_H
#define ESP32_WIIMOTE_HCI_UTILS_H

#include <stdint.h>
#include <string.h>

/**
 * HCI Utilities - Byte stream operations and formatting
 */

// Bluetooth address length
static constexpr int kBdAddrLen = 6;

/**
 * Byte stream manipulation helpers
 */
static inline void streamU16ToLe(uint8_t *&p, uint16_t value) {
    *p++ = static_cast<uint8_t>(value);
    *p++ = static_cast<uint8_t>(value >> 8);
}

static inline void streamU8ToLe(uint8_t *&p, uint8_t value) {
    *p++ = value;
}

static inline void streamBdAddr(uint8_t *&p, const uint8_t *bdAddr) {
    for (int i = 0; i < kBdAddrLen; i++) {
        *p++ = bdAddr[kBdAddrLen - 1 - i];
    }
}

static inline void streamToBdAddr(uint8_t *bdAddr, const uint8_t *p) {
    for (int i = 0; i < kBdAddrLen; i++) {
        bdAddr[kBdAddrLen - 1 - i] = p[i];
    }
}

static inline void streamArray(uint8_t *&p, const uint8_t *array, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        *p++ = array[i];
    }
}

/**
 * Byte extraction utilities
 * Read multi-byte values from byte arrays
 */

/**
 * Read 16-bit little-endian value (low byte first)
 * @param data Pointer to byte array
 * @return 16-bit value
 */
static inline uint16_t readUInt16Le(const uint8_t *data) {
    return ((uint16_t)data[1] << 8) | data[0];
}

/**
 * Read 16-bit big-endian value (high byte first)
 * @param data Pointer to byte array
 * @return 16-bit value
 */
static inline uint16_t readUInt16Be(const uint8_t *data) {
    return ((uint16_t)data[0] << 8) | data[1];
}

/**
 * Bluetooth address structure
 */
struct BdAddrT {
    uint8_t addr[kBdAddrLen];
};

/**
 * H4 (UART) packet type identifiers
 */
enum class H4PacketType : uint8_t { Command = 1, Acl = 2, Sco = 3, Event = 4 };

/**
 * HCI packet size constants
 */
static constexpr uint16_t kHciH4CmdPreambleSize = 4;
static constexpr uint16_t kHciH4AclPreambleSize = 5;
static constexpr uint16_t kL2CapHeaderLen = 4;

#endif  // ESP32_WIIMOTE_HCI_UTILS_H
