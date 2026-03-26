// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "wiimote_protocol.h"

#include "../../utils/protocol_codes.h"
#include "../../utils/serial_logging.h"
#include "../l2cap/l2cap_connection.h"
#include "../l2cap/l2cap_packets.h"
#include "../utils/payload_builder.h"

#include <Arduino.h>

#include <string.h>

namespace {
constexpr uint8_t kWiimoteRptSetLeds = static_cast<uint8_t>(WiimoteOutputReport::SetLeds);
constexpr uint8_t kWiimoteRptSetReportingMode =
    static_cast<uint8_t>(WiimoteOutputReport::SetReportingMode);
constexpr uint8_t kWiimoteRptRequestStatus =
    static_cast<uint8_t>(WiimoteOutputReport::RequestStatus);
constexpr uint8_t kWiimoteRptWriteMemory = static_cast<uint8_t>(WiimoteOutputReport::WriteMemory);
constexpr uint8_t kWiimoteRptReadMemory = static_cast<uint8_t>(WiimoteOutputReport::ReadMemory);
constexpr uint8_t kHidOutputReport = static_cast<uint8_t>(WiimoteHidPrefix::OutputReport);
constexpr uint8_t kEepromDataSize = 16;
}  // namespace

WiimoteProtocol::WiimoteProtocol() : connections_(nullptr), sender_(nullptr), payload_{0} {}

void WiimoteProtocol::init(const L2capConnectionTable *connectionTable,
                           L2capPacketSender *packetSender) {
    connections_ = connectionTable;
    sender_ = packetSender;
}

bool WiimoteProtocol::isValidMemorySize(uint16_t size, const char *operation) {
    if (size > kEepromDataSize) {
        wiimoteLogError("%s size %d exceeds maximum %d\n", operation, size, kEepromDataSize);
        return false;
    }
    return true;
}

/**
 * Set Wiimote player LEDs
 *
 * Sends output report (0xA2 0x11 LL) where LL contains LED bits shifted left 4
 */
void WiimoteProtocol::setLeds(uint16_t ch, const WiimoteLedCommand &command) {
    wiimoteLogDebug("wiimote_set_leds\n");

    if (connections_ == nullptr || sender_ == nullptr) {
        return;
    }

    uint16_t remoteCID = 0;
    if (connections_->getRemoteCid(ch, &remoteCID) != 0) {
        wiimoteLogError("L2CAP connection not found\n");
        return;
    }

    // Build output report: A2 11 LL
    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append(kHidOutputReport);              // 0xA2
    pb.append(kWiimoteRptSetLeds);            // 0x11
    pb.append((uint8_t)(command.leds << 4));  // LED bits in high nibble

    sender_->sendAclL2capPacket(ch, remoteCID, payload_, pb.length());
    wiimoteLogDebug("queued acl_l2cap_packet(%s, leds=0x%02X)\n",
              wiimoteOutputReportToString(kWiimoteRptSetLeds), command.leds);
}

/**
 * Set Wiimote data reporting mode
 *
 * Sends output report (0xA2 0x12 TT MM) where:
 * - TT is continuous flag (0x00 or 0x04)
 * - MM is reporting mode
 */
void WiimoteProtocol::setReportingMode(uint16_t ch, const WiimoteReportingModeCommand &command) {
    wiimoteLogDebug("wiimote_set_reporting_mode mode=0x%02X (%s) continuous=%d\n", command.mode,
              wiimoteReportingModeToString(command.mode), command.continuous);

    if (connections_ == nullptr || sender_ == nullptr) {
        return;
    }

    uint16_t remoteCID = 0;
    if (connections_->getRemoteCid(ch, &remoteCID) != 0) {
        wiimoteLogError("L2CAP connection not found\n");
        return;
    }

    uint8_t contReportIsDesired = command.continuous ? 0x04 : 0x00;

    // Build output report: A2 12 TT MM
    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append(kHidOutputReport);             // 0xA2
    pb.append(kWiimoteRptSetReportingMode);  // 0x12
    pb.append(contReportIsDesired);          // Continuous flag
    pb.append(command.mode);                 // Reporting mode

    sender_->sendAclL2capPacket(ch, remoteCID, payload_, pb.length());
    wiimoteLogDebug("queued acl_l2cap_packet(Set Reporting Mode)\n");
}

/**
 * Request Wiimote status report
 *
 * Sends output report (0xA2 0x15 00) to request status information
 * Response comes as input report (0x20) with battery level and extension info
 */
void WiimoteProtocol::requestStatus(uint16_t ch) {
    wiimoteLogDebug("wiimote_request_status\n");

    if (connections_ == nullptr || sender_ == nullptr) {
        return;
    }

    uint16_t remoteCID = 0;
    if (connections_->getRemoteCid(ch, &remoteCID) != 0) {
        wiimoteLogError("L2CAP connection not found\n");
        return;
    }

    // Build output report: A2 15 00
    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append(kHidOutputReport);          // 0xA2
    pb.append(kWiimoteRptRequestStatus);  // 0x15
    pb.append(0x00);                      // No rumble

    sender_->sendAclL2capPacket(ch, remoteCID, payload_, pb.length());
    wiimoteLogDebug("queued acl_l2cap_packet(%s)\n",
              wiimoteOutputReportToString(kWiimoteRptRequestStatus));
}

/**
 * Write to Wiimote memory or control registers
 *
 * Sends output report (0xA2 0x16 MM OOOOOO SS DD..DD)
 * where MM is address space, O is offset, S is size, DD is data
 */
void WiimoteProtocol::writeMemory(uint16_t ch,
                                  WiimoteAddressSpace addressSpace,
                                  uint32_t offset,
                                  const uint8_t *data,
                                  uint8_t length) {
    const uint8_t kAddressSpaceByte = static_cast<uint8_t>(addressSpace);
    wiimoteLogDebug("wiimote_write_memory addr_space=%d (%s) offset=0x%06lX len=%d\n", kAddressSpaceByte,
              wiimoteAddressSpaceToString(kAddressSpaceByte), offset, length);

    if (connections_ == nullptr || sender_ == nullptr) {
        return;
    }

    if (!isValidMemorySize(length, "Write")) {
        return;
    }

    uint16_t remoteCID = 0;
    if (connections_->getRemoteCid(ch, &remoteCID) != 0) {
        wiimoteLogError("L2CAP connection not found\n");
        return;
    }

    // Build output report header
    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append(kHidOutputReport);        // 0xA2
    pb.append(kWiimoteRptWriteMemory);  // 0x16
    pb.append(kAddressSpaceByte);       // MM
    pb.appendU24BE(offset);             // 24-bit offset (big-endian)
    pb.append(length);                  // Length of data to write

    // Clear data area and copy user data
    pb.reserveZeroed(kEepromDataSize);
    memcpy(&payload_[pb.length() - kEepromDataSize], data, length);

    sender_->sendAclL2capPacket(ch, remoteCID, payload_, pb.length());
    wiimoteLogDebug("queued acl_l2cap_packet(Write Memory)\n");
}

/**
 * Read from Wiimote memory or control registers
 *
 * Sends output report (0xA2 0x17 MM OOOOOO SSSS)
 * Response comes as input report (0x21)
 */
void WiimoteProtocol::readMemory(uint16_t ch,
                                 WiimoteAddressSpace addressSpace,
                                 uint32_t offset,
                                 uint16_t size) {
    const uint8_t kAddressSpaceByte = static_cast<uint8_t>(addressSpace);
    wiimoteLogDebug("wiimote_read_memory addr_space=%d (%s) offset=0x%06lX size=%d\n", kAddressSpaceByte,
              wiimoteAddressSpaceToString(kAddressSpaceByte), offset, size);

    if (connections_ == nullptr || sender_ == nullptr) {
        return;
    }

    if (!isValidMemorySize(size, "Read")) {
        return;
    }

    uint16_t remoteCID = 0;
    if (connections_->getRemoteCid(ch, &remoteCID) != 0) {
        wiimoteLogError("L2CAP connection not found\n");
        return;
    }

    // Build output report
    PayloadBuilder pb(payload_, sizeof(payload_));
    pb.append(kHidOutputReport);       // 0xA2
    pb.append(kWiimoteRptReadMemory);  // 0x17
    pb.append(kAddressSpaceByte);      // MM
    pb.appendU24BE(offset);            // 24-bit offset (big-endian)
    pb.appendU16BE(size);              // 16-bit size (big-endian)

    sender_->sendAclL2capPacket(ch, remoteCID, payload_, pb.length());
    wiimoteLogDebug("queued acl_l2cap_packet(Read Memory)\n");
}
