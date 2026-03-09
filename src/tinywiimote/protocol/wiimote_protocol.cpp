// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "wiimote_protocol.h"
#include "../l2cap/l2cap_connection.h"
#include "../l2cap/l2cap_packets.h"
#include <string.h>
#include <Arduino.h>

// Debug macro
#define WIIMOTE_VERBOSE 0

#if WIIMOTE_VERBOSE
#define VERBOSE_PRINT(...) Serial.printf(__VA_ARGS__)
#define VERBOSE_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define VERBOSE_PRINT(...) do {} while(0)
#define VERBOSE_PRINTLN(...) do {} while(0)
#endif

/**
 * Wiimote output report packet buffer
 */
#define L2CAP_PAYLOAD_MAX_LEN (64)
static uint8_t wiimotePayload[L2CAP_PAYLOAD_MAX_LEN];

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

/**
 * Set Wiimote player LEDs
 * 
 * Sends output report (0xA2 0x11 LL) where LL contains LED bits shifted left 4
 */
void wiimote_set_leds(uint16_t ch, uint8_t leds) {
  VERBOSE_PRINTLN("wiimote_set_leds");

  uint16_t remoteCID = 0;
  if (l2cap_get_remote_cid(ch, &remoteCID) != 0) {
    VERBOSE_PRINTLN("ERROR: L2CAP connection not found");
    return;
  }

  // Build output report: A2 11 LL
  uint8_t posi = 0;
  wiimotePayload[posi++] = HID_OUTPUT_REPORT;           // 0xA2
  wiimotePayload[posi++] = WIIMOTE_RPT_SET_LEDS;        // 0x11
  wiimotePayload[posi++] = (uint8_t)(leds << 4);        // LED bits in high nibble

  uint16_t dataLen = posi;
  send_acl_l2cap_packet(ch, remoteCID, wiimotePayload, dataLen);
  VERBOSE_PRINT("queued acl_l2cap_packet(Set LEDs, leds=0x%02X)\n", leds);
}

/**
 * Set Wiimote data reporting mode
 * 
 * Sends output report (0xA2 0x12 TT MM) where:
 * - TT is continuous flag (0x00 or 0x04)
 * - MM is reporting mode
 */
void wiimote_set_reporting_mode(uint16_t ch, uint8_t mode, bool continuous) {
  VERBOSE_PRINT("wiimote_set_reporting_mode mode=0x%02X continuous=%d\n", mode, continuous);

  uint16_t remoteCID = 0;
  if (l2cap_get_remote_cid(ch, &remoteCID) != 0) {
    VERBOSE_PRINTLN("ERROR: L2CAP connection not found");
    return;
  }

  uint8_t contReportIsDesired = continuous ? 0x04 : 0x00;

  // Build output report: A2 12 TT MM
  uint8_t posi = 0;
  wiimotePayload[posi++] = HID_OUTPUT_REPORT;           // 0xA2
  wiimotePayload[posi++] = WIIMOTE_RPT_SET_REPORTING_MODE; // 0x12
  wiimotePayload[posi++] = contReportIsDesired;         // Continuous flag
  wiimotePayload[posi++] = mode;                        // Reporting mode

  uint16_t dataLen = posi;
  send_acl_l2cap_packet(ch, remoteCID, wiimotePayload, dataLen);
  VERBOSE_PRINTLN("queued acl_l2cap_packet(Set Reporting Mode)");
}

/**
 * Write to Wiimote memory or control registers
 * 
 * Sends output report (0xA2 0x16 MM OOOOOO SS DD..DD)
 * where MM is address space, O is offset, S is size, DD is data
 */
void wiimote_write_memory(uint16_t ch, address_space_t address_space, uint32_t offset,
                          const uint8_t* data, uint8_t length) {
  VERBOSE_PRINT("wiimote_write_memory addr_space=%d offset=0x%06lX len=%d\n", 
                address_space, offset, length);
  
  if(length > EEPROM_DATA_SIZE) {
    VERBOSE_PRINT("ERROR: Write length %d exceeds maximum %d\n", length, EEPROM_DATA_SIZE);
    return;
  }

  uint16_t remoteCID = 0;
  if (l2cap_get_remote_cid(ch, &remoteCID) != 0) {
    VERBOSE_PRINTLN("ERROR: L2CAP connection not found");
    return;
  }

  // Build output report header
  uint8_t posi = 0;
  wiimotePayload[posi++] = HID_OUTPUT_REPORT;           // 0xA2
  wiimotePayload[posi++] = WIIMOTE_RPT_WRITE_MEMORY;    // 0x16
  wiimotePayload[posi++] = get_address_space_byte(address_space); // MM
  
  // 24-bit offset (big-endian)
  wiimotePayload[posi++] = (uint8_t)((offset >> 16) & 0xFF);
  wiimotePayload[posi++] = (uint8_t)((offset >>  8) & 0xFF);
  wiimotePayload[posi++] = (uint8_t)((offset      ) & 0xFF);
  
  // Length of data to write
  wiimotePayload[posi++] = length;
  
  // Clear data area and copy user data
  memset(&wiimotePayload[posi], 0, EEPROM_DATA_SIZE);
  memcpy(&wiimotePayload[posi], data, length);
  posi += EEPROM_DATA_SIZE;

  uint16_t dataLen = posi;
  send_acl_l2cap_packet(ch, remoteCID, wiimotePayload, dataLen);
  VERBOSE_PRINTLN("queued acl_l2cap_packet(Write Memory)");
}

/**
 * Read from Wiimote memory or control registers
 * 
 * Sends output report (0xA2 0x17 MM OOOOOO SSSS)
 * Response comes as input report (0x21)
 */
void wiimote_read_memory(uint16_t ch, address_space_t address_space, uint32_t offset, uint16_t size) {
  VERBOSE_PRINT("wiimote_read_memory addr_space=%d offset=0x%06lX size=%d\n", 
                address_space, offset, size);
  
  if(size > EEPROM_DATA_SIZE) {
    VERBOSE_PRINT("ERROR: Read size %d exceeds maximum %d\n", size, EEPROM_DATA_SIZE);
    return;
  }

  uint16_t remoteCID = 0;
  if (l2cap_get_remote_cid(ch, &remoteCID) != 0) {
    VERBOSE_PRINTLN("ERROR: L2CAP connection not found");
    return;
  }

  // Build output report
  uint8_t posi = 0;
  wiimotePayload[posi++] = HID_OUTPUT_REPORT;           // 0xA2
  wiimotePayload[posi++] = WIIMOTE_RPT_READ_MEMORY;     // 0x17
  wiimotePayload[posi++] = get_address_space_byte(address_space); // MM
  
  // 24-bit offset (big-endian)
  wiimotePayload[posi++] = (uint8_t)((offset >> 16) & 0xFF);
  wiimotePayload[posi++] = (uint8_t)((offset >>  8) & 0xFF);
  wiimotePayload[posi++] = (uint8_t)((offset      ) & 0xFF);
  
  // 16-bit size (big-endian)
  wiimotePayload[posi++] = (uint8_t)((size >> 8) & 0xFF);
  wiimotePayload[posi++] = (uint8_t)((size     ) & 0xFF);

  uint16_t dataLen = posi;
  send_acl_l2cap_packet(ch, remoteCID, wiimotePayload, dataLen);
  VERBOSE_PRINTLN("queued acl_l2cap_packet(Read Memory)");
}
