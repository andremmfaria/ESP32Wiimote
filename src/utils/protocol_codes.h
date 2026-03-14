// Copyright (c) 2026 ESP32Wiimote contributors
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32WIIMOTE_UTILS_PROTOCOL_CODES_H_
#define ESP32WIIMOTE_UTILS_PROTOCOL_CODES_H_

#include <stdint.h>

enum class L2capSignalingCode : uint8_t {
    CONNECTION_REQUEST = 0x02,
    CONNECTION_RESPONSE = 0x03,
    CONFIGURATION_REQUEST = 0x04,
    CONFIGURATION_RESPONSE = 0x05,
};

enum class L2capSignalingResult : uint16_t {
    SUCCESS = 0x0000,
    PENDING = 0x0001,
    PSM_NOT_SUPPORTED = 0x0002,
    SECURITY_BLOCK = 0x0003,
    NO_RESOURCES = 0x0004,
};

enum class L2capCid : uint16_t {
    SIGNALING = 0x0001,
};

enum class L2capPsm : uint16_t {
    HID_CONTROL = 0x0011,
    HID_INTERRUPT = 0x0013,
};

enum class WiimoteHidPrefix : uint8_t {
    INPUT_REPORT = 0xA1,
    OUTPUT_REPORT = 0xA2,
};

enum class WiimoteInputReport : uint8_t {
    STATUS_INFORMATION = 0x20,
    READ_MEMORY_DATA = 0x21,
    CORE_BUTTONS = 0x30,
    CORE_BUTTONS_ACCEL = 0x31,
    CORE_BUTTONS_EXT8 = 0x32,
    CORE_BUTTONS_ACCEL_EXT16 = 0x35,
};

enum class WiimoteOutputReport : uint8_t {
    SET_LEDS = 0x11,
    SET_REPORTING_MODE = 0x12,
    REQUEST_STATUS = 0x15,
    WRITE_MEMORY = 0x16,
    READ_MEMORY = 0x17,
};

enum class WiimoteAddressSpace : uint8_t {
    EEPROM = 0x00,
    CONTROL_REGISTER = 0x04,
};

enum class BtControllerStatusCode : uint8_t {
    IDLE = 0,
    INITED = 1,
    ENABLED = 2,
};

const char *l2capSignalCodeToString(uint8_t code);
const char *l2capSignalingResultToString(uint16_t result);
const char *wiimoteHidPrefixToString(uint8_t prefix);
const char *wiimoteInputReportToString(uint8_t report);
const char *wiimoteOutputReportToString(uint8_t report);
const char *wiimoteAddressSpaceToString(uint8_t addressSpace);
const char *wiimoteReportingModeToString(uint8_t mode);
const char *btControllerStatusToString(uint8_t status);

#endif  // ESP32WIIMOTE_UTILS_PROTOCOL_CODES_H_
