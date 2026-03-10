// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __WIIMOTE_PROTOCOL_H__
#define __WIIMOTE_PROTOCOL_H__

#include "../l2cap/l2cap_connection.h"
#include "../l2cap/l2cap_packets.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * Wiimote Protocol - Wiimote-specific commands and operations
 *
 * Handles high-level Wiimote operations:
 * - LED control
 * - Data reporting mode configuration
 * - Memory/register read/write operations
 */

/**
 * Memory address space types
 */
typedef enum { EEPROM_MEMORY = 0x00, CONTROL_REGISTER = 0x04 } address_space_t;

class WiimoteProtocol {
   public:
    WiimoteProtocol();

    void init(const L2capConnectionTable *connectionTable, L2capPacketSender *sender);

    void setLeds(uint16_t ch, uint8_t leds);
    void setReportingMode(uint16_t ch, uint8_t mode, bool continuous);
    void requestStatus(uint16_t ch);
    void writeMemory(uint16_t ch,
                     address_space_t address_space,
                     uint32_t offset,
                     const uint8_t *data,
                     uint8_t length);
    void readMemory(uint16_t ch, address_space_t address_space, uint32_t offset, uint16_t size);

   private:
    /**
     * Validate memory operation size
     * @param size Size to validate
     * @param operation Operation name for error message ("Write" or "Read")
     * @return true if valid, false if exceeds maximum
     */
    static bool isValidMemorySize(uint16_t size, const char *operation);

    const L2capConnectionTable *connections;
    L2capPacketSender *sender;
    uint8_t payload[64];
};

#endif  // __WIIMOTE_PROTOCOL_H__
