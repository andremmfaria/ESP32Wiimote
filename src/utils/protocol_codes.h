// Copyright (c) 2026 ESP32Wiimote contributors
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_UTILS_PROTOCOL_CODES_H
#define ESP32_WIIMOTE_UTILS_PROTOCOL_CODES_H

#include <stdint.h>

enum class L2capSignalingCode : uint8_t {
    ConnectionRequest = 0x02,
    ConnectionResponse = 0x03,
    ConfigurationRequest = 0x04,
    ConfigurationResponse = 0x05,
};

enum class L2capSignalingResult : uint16_t {
    SUCCESS = 0x0000,
    PENDING = 0x0001,
    PsmNotSupported = 0x0002,
    SecurityBlock = 0x0003,
    NoResources = 0x0004,
};

enum class L2capCid : uint16_t {
    SIGNALING = 0x0001,
};

enum class L2capPsm : uint16_t {
    HidControl = 0x0011,
    HidInterrupt = 0x0013,
};

enum class WiimoteHidPrefix : uint8_t {
    InputReport = 0xA1,
    OutputReport = 0xA2,
};

enum class WiimoteInputReport : uint8_t {
    StatusInformation = 0x20,
    ReadMemoryData = 0x21,
    CoreButtons = 0x30,
    CoreButtonsAccel = 0x31,
    CoreButtonsExT8 = 0x32,
    CoreButtonsAccelExT16 = 0x35,
};

enum class WiimoteOutputReport : uint8_t {
    SetLeds = 0x11,
    SetReportingMode = 0x12,
    RequestStatus = 0x15,
    WriteMemory = 0x16,
    ReadMemory = 0x17,
};

enum class WiimoteAddressSpace : uint8_t {
    EEPROM = 0x00,
    ControlRegister = 0x04,
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

#endif  // ESP32_WIIMOTE_UTILS_PROTOCOL_CODES_H
