// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "wiimote_protocol.h"
#include "../l2cap/l2cap_connection.h"
#include "../l2cap/l2cap_packets.h"
#include "../utils/payload_builder.h"
#include <string.h>
#include <Arduino.h>
#include "../../utils/serial_logging.h"

/**
 * Wiimote output report opcodes
 */
#define WIIMOTE_RPT_SET_LEDS              0x11
#define WIIMOTE_RPT_SET_REPORTING_MODE    0x12
#define WIIMOTE_RPT_WRITE_MEMORY          0x16
#define WIIMOTE_RPT_READ_MEMORY           0x17

/**
 * HID output report prefix
 */
#define HID_OUTPUT_REPORT                 0xA2

/**
 * Memory write/read constraints
 */
#define EEPROM_DATA_SIZE                  (16)

/**
 * Convert address space enum to Wiimote address space byte
 * 
 * @param address_space address_space_t enum value
 * @return Address space byte (0x00 for EEPROM, 0x04 for Control Register)
 */
static uint8_t get_address_space_byte(address_space_t address_space) {
  switch(address_space) {
    case EEPROM_MEMORY:
      return 0x00;
    case CONTROL_REGISTER:
      return 0x04;
    default:
      return 0xFF;
  }
}

WiimoteProtocol::WiimoteProtocol() : connections(nullptr), sender(nullptr), payload{0} {}

void WiimoteProtocol::init(const L2capConnectionTable* connectionTable,
                           L2capPacketSender* packetSender) {
  connections = connectionTable;
  sender = packetSender;
}

bool WiimoteProtocol::isValidMemorySize(uint16_t size, const char* operation) const {
  if (size > EEPROM_DATA_SIZE) {
    LOG_ERROR("%s size %d exceeds maximum %d\n", operation, size, EEPROM_DATA_SIZE);
    return false;
  }
  return true;
}

/**
 * Set Wiimote player LEDs
 * 
 * Sends output report (0xA2 0x11 LL) where LL contains LED bits shifted left 4
 */
void WiimoteProtocol::setLeds(uint16_t ch, uint8_t leds) {
  LOG_DEBUG("wiimote_set_leds\n");

  if (connections == nullptr || sender == nullptr) {
    return;
  }

  uint16_t remoteCID = 0;
  if (connections->getRemoteCid(ch, &remoteCID) != 0) {
    LOG_ERROR("L2CAP connection not found\n");
    return;
  }

  // Build output report: A2 11 LL
  PayloadBuilder pb(payload, sizeof(payload));
  pb.append(HID_OUTPUT_REPORT);           // 0xA2
  pb.append(WIIMOTE_RPT_SET_LEDS);        // 0x11
  pb.append((uint8_t)(leds << 4));        // LED bits in high nibble

  sender->sendAclL2capPacket(ch, remoteCID, payload, pb.length());
  LOG_DEBUG("queued acl_l2cap_packet(Set LEDs, leds=0x%02X)\n", leds);
}

/**
 * Set Wiimote data reporting mode
 * 
 * Sends output report (0xA2 0x12 TT MM) where:
 * - TT is continuous flag (0x00 or 0x04)
 * - MM is reporting mode
 */
void WiimoteProtocol::setReportingMode(uint16_t ch, uint8_t mode, bool continuous) {
  LOG_DEBUG("wiimote_set_reporting_mode mode=0x%02X continuous=%d\n", mode, continuous);

  if (connections == nullptr || sender == nullptr) {
    return;
  }

  uint16_t remoteCID = 0;
  if (connections->getRemoteCid(ch, &remoteCID) != 0) {
    LOG_ERROR("L2CAP connection not found\n");
    return;
  }

  uint8_t contReportIsDesired = continuous ? 0x04 : 0x00;

  // Build output report: A2 12 TT MM
  PayloadBuilder pb(payload, sizeof(payload));
  pb.append(HID_OUTPUT_REPORT);           // 0xA2
  pb.append(WIIMOTE_RPT_SET_REPORTING_MODE); // 0x12
  pb.append(contReportIsDesired);         // Continuous flag
  pb.append(mode);                        // Reporting mode

  sender->sendAclL2capPacket(ch, remoteCID, payload, pb.length());
  LOG_DEBUG("queued acl_l2cap_packet(Set Reporting Mode)\n");
}

/**
 * Write to Wiimote memory or control registers
 * 
 * Sends output report (0xA2 0x16 MM OOOOOO SS DD..DD)
 * where MM is address space, O is offset, S is size, DD is data
 */
void WiimoteProtocol::writeMemory(uint16_t ch, address_space_t address_space,
                                  uint32_t offset, const uint8_t* data,
                                  uint8_t length) {
  LOG_DEBUG("wiimote_write_memory addr_space=%d offset=0x%06lX len=%d\n", 
                address_space, offset, length);

  if (connections == nullptr || sender == nullptr) {
    return;
  }
  
  if (!isValidMemorySize(length, "Write")) {
    return;
  }

  uint16_t remoteCID = 0;
  if (connections->getRemoteCid(ch, &remoteCID) != 0) {
    LOG_ERROR("L2CAP connection not found\n");
    return;
  }

  // Build output report header
  PayloadBuilder pb(payload, sizeof(payload));
  pb.append(HID_OUTPUT_REPORT);           // 0xA2
  pb.append(WIIMOTE_RPT_WRITE_MEMORY);    // 0x16
  pb.append(get_address_space_byte(address_space)); // MM
  pb.appendU24BE(offset);                 // 24-bit offset (big-endian)
  pb.append(length);                      // Length of data to write
  
  // Clear data area and copy user data
  pb.reserveZeroed(EEPROM_DATA_SIZE);
  memcpy(&payload[pb.length() - EEPROM_DATA_SIZE], data, length);

  sender->sendAclL2capPacket(ch, remoteCID, payload, pb.length());
  LOG_DEBUG("queued acl_l2cap_packet(Write Memory)\n");
}

/**
 * Read from Wiimote memory or control registers
 * 
 * Sends output report (0xA2 0x17 MM OOOOOO SSSS)
 * Response comes as input report (0x21)
 */
void WiimoteProtocol::readMemory(uint16_t ch, address_space_t address_space,
                                 uint32_t offset, uint16_t size) {
  LOG_DEBUG("wiimote_read_memory addr_space=%d offset=0x%06lX size=%d\n", 
                address_space, offset, size);

  if (connections == nullptr || sender == nullptr) {
    return;
  }
  
  if (!isValidMemorySize(size, "Read")) {
    return;
  }

  uint16_t remoteCID = 0;
  if (connections->getRemoteCid(ch, &remoteCID) != 0) {
    LOG_ERROR("L2CAP connection not found\n");
    return;
  }

  // Build output report
  PayloadBuilder pb(payload, sizeof(payload));
  pb.append(HID_OUTPUT_REPORT);           // 0xA2
  pb.append(WIIMOTE_RPT_READ_MEMORY);     // 0x17
  pb.append(get_address_space_byte(address_space)); // MM
  pb.appendU24BE(offset);                 // 24-bit offset (big-endian)
  pb.appendU16BE(size);                   // 16-bit size (big-endian)

  sender->sendAclL2capPacket(ch, remoteCID, payload, pb.length());
  LOG_DEBUG("queued acl_l2cap_packet(Read Memory)\n");
}
