// Copyright (c) 2026 ESP32Wiimote contributors
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "protocol_codes.h"

const char *l2capSignalCodeToString(uint8_t code) {
    switch (static_cast<L2capSignalingCode>(code)) {
        case L2capSignalingCode::CONNECTION_REQUEST:
            return "CONNECTION_REQUEST";
        case L2capSignalingCode::CONNECTION_RESPONSE:
            return "CONNECTION_RESPONSE";
        case L2capSignalingCode::CONFIGURATION_REQUEST:
            return "CONFIGURATION_REQUEST";
        case L2capSignalingCode::CONFIGURATION_RESPONSE:
            return "CONFIGURATION_RESPONSE";
        default:
            return "UNKNOWN_SIGNAL";
    }
}

const char *l2capSignalingResultToString(uint16_t result) {
    switch (static_cast<L2capSignalingResult>(result)) {
        case L2capSignalingResult::SUCCESS:
            return "SUCCESS";
        case L2capSignalingResult::PENDING:
            return "PENDING";
        case L2capSignalingResult::PSM_NOT_SUPPORTED:
            return "PSM_NOT_SUPPORTED";
        case L2capSignalingResult::SECURITY_BLOCK:
            return "SECURITY_BLOCK";
        case L2capSignalingResult::NO_RESOURCES:
            return "NO_RESOURCES";
        default:
            return "UNKNOWN_RESULT";
    }
}

const char *wiimoteHidPrefixToString(uint8_t prefix) {
    switch (static_cast<WiimoteHidPrefix>(prefix)) {
        case WiimoteHidPrefix::INPUT_REPORT:
            return "INPUT_REPORT";
        case WiimoteHidPrefix::OUTPUT_REPORT:
            return "OUTPUT_REPORT";
        default:
            return "UNKNOWN_HID_PREFIX";
    }
}

const char *wiimoteInputReportToString(uint8_t report) {
    switch (static_cast<WiimoteInputReport>(report)) {
        case WiimoteInputReport::STATUS_INFORMATION:
            return "STATUS_INFORMATION";
        case WiimoteInputReport::READ_MEMORY_DATA:
            return "READ_MEMORY_DATA";
        case WiimoteInputReport::CORE_BUTTONS:
            return "CORE_BUTTONS";
        case WiimoteInputReport::CORE_BUTTONS_ACCEL:
            return "CORE_BUTTONS_ACCEL";
        case WiimoteInputReport::CORE_BUTTONS_EXT8:
            return "CORE_BUTTONS_EXT8";
        case WiimoteInputReport::CORE_BUTTONS_ACCEL_EXT16:
            return "CORE_BUTTONS_ACCEL_EXT16";
        default:
            return "UNKNOWN_INPUT_REPORT";
    }
}

const char *wiimoteOutputReportToString(uint8_t report) {
    switch (static_cast<WiimoteOutputReport>(report)) {
        case WiimoteOutputReport::SET_LEDS:
            return "SET_LEDS";
        case WiimoteOutputReport::SET_REPORTING_MODE:
            return "SET_REPORTING_MODE";
        case WiimoteOutputReport::REQUEST_STATUS:
            return "REQUEST_STATUS";
        case WiimoteOutputReport::WRITE_MEMORY:
            return "WRITE_MEMORY";
        case WiimoteOutputReport::READ_MEMORY:
            return "READ_MEMORY";
        default:
            return "UNKNOWN_OUTPUT_REPORT";
    }
}

const char *wiimoteAddressSpaceToString(uint8_t addressSpace) {
    switch (static_cast<WiimoteAddressSpace>(addressSpace)) {
        case WiimoteAddressSpace::EEPROM:
            return "EEPROM";
        case WiimoteAddressSpace::CONTROL_REGISTER:
            return "CONTROL_REGISTER";
        default:
            return "UNKNOWN_ADDRESS_SPACE";
    }
}

const char *wiimoteReportingModeToString(uint8_t mode) {
    switch (mode) {
        case 0x30:
            return "CORE_BUTTONS";
        case 0x31:
            return "CORE_BUTTONS_ACCEL";
        case 0x32:
            return "CORE_BUTTONS_EXT8";
        case 0x33:
            return "CORE_BUTTONS_ACCEL_IR12";
        case 0x34:
            return "CORE_BUTTONS_EXT19";
        case 0x35:
            return "CORE_BUTTONS_ACCEL_EXT16";
        case 0x36:
            return "CORE_BUTTONS_IR10_EXT9";
        case 0x37:
            return "CORE_BUTTONS_ACCEL_IR10_EXT6";
        default:
            return "UNKNOWN_REPORTING_MODE";
    }
}

const char *btControllerStatusToString(uint8_t status) {
    switch (static_cast<BtControllerStatusCode>(status)) {
        case BtControllerStatusCode::IDLE:
            return "IDLE";
        case BtControllerStatusCode::INITED:
            return "INITED";
        case BtControllerStatusCode::ENABLED:
            return "ENABLED";
        default:
            return "UNKNOWN_STATUS";
    }
}
