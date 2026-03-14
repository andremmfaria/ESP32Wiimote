// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __HCI_UTILS_H__
#define __HCI_UTILS_H__

#include <stdint.h>
#include <string.h>

/**
 * HCI Utilities - Byte stream operations and formatting
 */

// Bluetooth address length
#define BD_ADDR_LEN (6)

/**
 * Byte stream manipulation macros
 */
#define UINT16_TO_STREAM(p, u16)        \
    {                                   \
        *(p)++ = (uint8_t)(u16);        \
        *(p)++ = (uint8_t)((u16) >> 8); \
    }

#define UINT8_TO_STREAM(p, u8)  \
    {                           \
        *(p)++ = (uint8_t)(u8); \
    }

#define BDADDR_TO_STREAM(p, a)                            \
    {                                                     \
        int ijk;                                          \
        for (ijk = 0; ijk < BD_ADDR_LEN; ijk++)           \
            *(p)++ = (uint8_t)(a)[BD_ADDR_LEN - 1 - ijk]; \
    }

#define STREAM_TO_BDADDR(a, p)                     \
    {                                              \
        int ijk;                                   \
        for (ijk = 0; ijk < BD_ADDR_LEN; ijk++)    \
            (a)[BD_ADDR_LEN - 1 - ijk] = (p)[ijk]; \
    }

#define ARRAY_TO_STREAM(p, a, len)        \
    {                                     \
        int ijk;                          \
        for (ijk = 0; ijk < (len); ijk++) \
            *(p)++ = (uint8_t)(a)[ijk];   \
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
static inline uint16_t READ_UINT16_LE(const uint8_t *data) {
    return ((uint16_t)data[1] << 8) | data[0];
}

/**
 * Read 16-bit big-endian value (high byte first)
 * @param data Pointer to byte array
 * @return 16-bit value
 */
static inline uint16_t READ_UINT16_BE(const uint8_t *data) {
    return ((uint16_t)data[0] << 8) | data[1];
}

/**
 * Bluetooth address structure
 */
struct bd_addr_t {
    uint8_t addr[BD_ADDR_LEN];
};

/**
 * H4 (UART) packet type identifiers
 */
enum { H4_TYPE_COMMAND = 1, H4_TYPE_ACL = 2, H4_TYPE_SCO = 3, H4_TYPE_EVENT = 4 };

/**
 * HCI packet size constants
 */
#define HCI_H4_CMD_PREAMBLE_SIZE (4)
#define HCI_H4_ACL_PREAMBLE_SIZE (5)
#define L2CAP_HEADER_LEN (4)

/**
 * Format data as hex string for debugging
 * @param data Pointer to byte data
 * @param len Length of data
 * @return Pointer to static hex string buffer
 */
char *format2Hex(uint8_t *data, uint16_t len);

#endif  // __HCI_UTILS_H__
