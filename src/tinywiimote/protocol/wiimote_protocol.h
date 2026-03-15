// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_WIIMOTE_PROTOCOL_H
#define ESP32_WIIMOTE_WIIMOTE_PROTOCOL_H

#include "../../utils/protocol_codes.h"
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

struct WiimoteLedCommand {
    uint8_t leds;
};

struct WiimoteReportingModeCommand {
    uint8_t mode;
    bool continuous;
};

class WiimoteProtocol {
   public:
    WiimoteProtocol();

    void init(const L2capConnectionTable *connectionTable, L2capPacketSender *sender);

    void setLeds(uint16_t ch, const WiimoteLedCommand &command);
    void setReportingMode(uint16_t ch, const WiimoteReportingModeCommand &command);
    void requestStatus(uint16_t ch);
    void writeMemory(uint16_t ch,
                     WiimoteAddressSpace addressSpace,
                     uint32_t offset,
                     const uint8_t *data,
                     uint8_t length);
    void readMemory(uint16_t ch, WiimoteAddressSpace addressSpace, uint32_t offset, uint16_t size);

   private:
    /**
     * Validate memory operation size
     * @param size Size to validate
     * @param operation Operation name for error message ("Write" or "Read")
     * @return true if valid, false if exceeds maximum
     */
    static bool isValidMemorySize(uint16_t size, const char *operation);

    const L2capConnectionTable *connections_;
    L2capPacketSender *sender_;
    uint8_t payload_[64];
};

#endif  // ESP32_WIIMOTE_WIIMOTE_PROTOCOL_H
