// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __WIIMOTE_PROTOCOL_H__
#define __WIIMOTE_PROTOCOL_H__

#include <stdint.h>
#include <stdbool.h>

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
typedef enum {
  EEPROM_MEMORY = 0x00,
  CONTROL_REGISTER = 0x04
} address_space_t;

/**
 * Set Wiimote player LEDs
 * 
 * Sends LED output report (0x11) to control the four player LEDs
 * 
 * @param ch Connection handle
 * @param leds 4-bit LED mask (bits 0-3 for LEDs 1-4)
 */
void wiimote_set_leds(uint16_t ch, uint8_t leds);

/**
 * Set Wiimote data reporting mode
 * 
 * Configures which data the Wiimote sends in reports
 * Common modes:
 * - 0x30: Core Buttons (5 bytes)
 * - 0x31: Core Buttons and Accelerometer (7 bytes)
 * - 0x32: Core Buttons with 8 Extension bytes
 * - 0x35: Core Buttons and Accel with 16 Extension bytes
 * 
 * @param ch Connection handle
 * @param mode Report mode byte
 * @param continuous True for continuous reporting, false for on-demand
 */
void wiimote_set_reporting_mode(uint16_t ch, uint8_t mode, bool continuous);

/**
 * Write to Wiimote memory or control registers
 * 
 * Sends write memory output report (0x16)
 * Writes up to 16 bytes at a time
 * 
 * @param ch Connection handle
 * @param address_space EEPROM_MEMORY or CONTROL_REGISTER
 * @param offset 24-bit memory offset
 * @param data Pointer to data to write
 * @param length Length of data (1-16 bytes)
 */
void wiimote_write_memory(uint16_t ch, address_space_t address_space, uint32_t offset,
                          const uint8_t* data, uint8_t length);

/**
 * Read from Wiimote memory or control registers
 * 
 * Sends read memory output report (0x17)
 * Returns data via input reports (0x21)
 * 
 * @param ch Connection handle
 * @param address_space EEPROM_MEMORY or CONTROL_REGISTER
 * @param offset 24-bit memory offset
 * @param size Number of bytes to read (1-16)
 */
void wiimote_read_memory(uint16_t ch, address_space_t address_space, uint32_t offset, uint16_t size);

#endif // __WIIMOTE_PROTOCOL_H__
