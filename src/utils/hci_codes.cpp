// Copyright (c) 2026 ESP32Wiimote contributors
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_codes.h"

const char *hciStatusCodeToString(uint8_t statusCode) {
    switch (static_cast<HciStatusCode>(statusCode)) {
        case HciStatusCode::SUCCESS:
            return "SUCCESS";
        case HciStatusCode::UNKNOWN_HCI_COMMAND:
            return "UNKNOWN_HCI_COMMAND";
        case HciStatusCode::UNKNOWN_CONNECTION_IDENTIFIER:
            return "UNKNOWN_CONNECTION_IDENTIFIER";
        case HciStatusCode::HARDWARE_FAILURE:
            return "HARDWARE_FAILURE";
        case HciStatusCode::PAGE_TIMEOUT:
            return "PAGE_TIMEOUT";
        case HciStatusCode::AUTHENTICATION_FAILURE:
            return "AUTHENTICATION_FAILURE";
        case HciStatusCode::PIN_OR_KEY_MISSING:
            return "PIN_OR_KEY_MISSING";
        case HciStatusCode::MEMORY_CAPACITY_EXCEEDED:
            return "MEMORY_CAPACITY_EXCEEDED";
        case HciStatusCode::CONNECTION_TIMEOUT:
            return "CONNECTION_TIMEOUT";
        case HciStatusCode::CONNECTION_LIMIT_EXCEEDED:
            return "CONNECTION_LIMIT_EXCEEDED";
        case HciStatusCode::SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED:
            return "SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED";
        case HciStatusCode::CONNECTION_ALREADY_EXISTS:
            return "CONNECTION_ALREADY_EXISTS";
        case HciStatusCode::COMMAND_DISALLOWED:
            return "COMMAND_DISALLOWED";
        case HciStatusCode::CONNECTION_REJECTED_LIMITED_RESOURCES:
            return "CONNECTION_REJECTED_LIMITED_RESOURCES";
        case HciStatusCode::CONNECTION_REJECTED_SECURITY:
            return "CONNECTION_REJECTED_SECURITY";
        case HciStatusCode::CONNECTION_REJECTED_BAD_BD_ADDR:
            return "CONNECTION_REJECTED_BAD_BD_ADDR";
        case HciStatusCode::CONNECTION_ACCEPT_TIMEOUT_EXCEEDED:
            return "CONNECTION_ACCEPT_TIMEOUT_EXCEEDED";
        case HciStatusCode::UNSUPPORTED_FEATURE_OR_PARAMETER:
            return "UNSUPPORTED_FEATURE_OR_PARAMETER";
        case HciStatusCode::INVALID_HCI_COMMAND_PARAMETERS:
            return "INVALID_HCI_COMMAND_PARAMETERS";
        case HciStatusCode::REMOTE_USER_TERMINATED_CONNECTION:
            return "REMOTE_USER_TERMINATED_CONNECTION";
        case HciStatusCode::REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES";
        case HciStatusCode::REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF";
        case HciStatusCode::CONNECTION_TERMINATED_BY_LOCAL_HOST:
            return "CONNECTION_TERMINATED_BY_LOCAL_HOST";
        case HciStatusCode::REPEATED_ATTEMPTS:
            return "REPEATED_ATTEMPTS";
        case HciStatusCode::PAIRING_NOT_ALLOWED:
            return "PAIRING_NOT_ALLOWED";
        case HciStatusCode::UNKNOWN_LMP_PDU:
            return "UNKNOWN_LMP_PDU";
        case HciStatusCode::UNSUPPORTED_REMOTE_FEATURE:
            return "UNSUPPORTED_REMOTE_FEATURE";
        case HciStatusCode::SCO_OFFSET_REJECTED:
            return "SCO_OFFSET_REJECTED";
        case HciStatusCode::SCO_INTERVAL_REJECTED:
            return "SCO_INTERVAL_REJECTED";
        case HciStatusCode::SCO_AIR_MODE_REJECTED:
            return "SCO_AIR_MODE_REJECTED";
        case HciStatusCode::INVALID_LMP_OR_LL_PARAMETERS:
            return "INVALID_LMP_OR_LL_PARAMETERS";
        case HciStatusCode::UNSPECIFIED_ERROR:
            return "UNSPECIFIED_ERROR";
        case HciStatusCode::UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE:
            return "UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE";
        case HciStatusCode::ROLE_CHANGE_NOT_ALLOWED:
            return "ROLE_CHANGE_NOT_ALLOWED";
        case HciStatusCode::LMP_OR_LL_RESPONSE_TIMEOUT:
            return "LMP_OR_LL_RESPONSE_TIMEOUT";
        case HciStatusCode::LMP_ERROR_TRANSACTION_COLLISION:
            return "LMP_ERROR_TRANSACTION_COLLISION";
        case HciStatusCode::LMP_PDU_NOT_ALLOWED:
            return "LMP_PDU_NOT_ALLOWED";
        case HciStatusCode::ENCRYPTION_MODE_NOT_ACCEPTABLE:
            return "ENCRYPTION_MODE_NOT_ACCEPTABLE";
        case HciStatusCode::LINK_KEY_CANNOT_BE_CHANGED:
            return "LINK_KEY_CANNOT_BE_CHANGED";
        case HciStatusCode::REQUESTED_QOS_NOT_SUPPORTED:
            return "REQUESTED_QOS_NOT_SUPPORTED";
        case HciStatusCode::INSTANT_PASSED:
            return "INSTANT_PASSED";
        case HciStatusCode::PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED:
            return "PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED";
        case HciStatusCode::DIFFERENT_TRANSACTION_COLLISION:
            return "DIFFERENT_TRANSACTION_COLLISION";
        case HciStatusCode::QOS_UNACCEPTABLE_PARAMETER:
            return "QOS_UNACCEPTABLE_PARAMETER";
        case HciStatusCode::QOS_REJECTED:
            return "QOS_REJECTED";
        case HciStatusCode::CHANNEL_CLASSIFICATION_NOT_SUPPORTED:
            return "CHANNEL_CLASSIFICATION_NOT_SUPPORTED";
        case HciStatusCode::INSUFFICIENT_SECURITY:
            return "INSUFFICIENT_SECURITY";
        case HciStatusCode::PARAMETER_OUT_OF_MANDATORY_RANGE:
            return "PARAMETER_OUT_OF_MANDATORY_RANGE";
        case HciStatusCode::ROLE_SWITCH_PENDING:
            return "ROLE_SWITCH_PENDING";
        case HciStatusCode::RESERVED_SLOT_VIOLATION:
            return "RESERVED_SLOT_VIOLATION";
        case HciStatusCode::ROLE_SWITCH_FAILED:
            return "ROLE_SWITCH_FAILED";
        case HciStatusCode::EXTENDED_INQUIRY_RESPONSE_TOO_LARGE:
            return "EXTENDED_INQUIRY_RESPONSE_TOO_LARGE";
        case HciStatusCode::SECURE_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST:
            return "SECURE_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST";
        case HciStatusCode::HOST_BUSY_PAIRING:
            return "HOST_BUSY_PAIRING";
        case HciStatusCode::CONNECTION_REJECTED_NO_SUITABLE_CHANNEL_FOUND:
            return "CONNECTION_REJECTED_NO_SUITABLE_CHANNEL_FOUND";
        case HciStatusCode::CONTROLLER_BUSY:
            return "CONTROLLER_BUSY";
        case HciStatusCode::UNACCEPTABLE_CONNECTION_PARAMETERS:
            return "UNACCEPTABLE_CONNECTION_PARAMETERS";
        case HciStatusCode::ADVERTISING_TIMEOUT:
            return "ADVERTISING_TIMEOUT";
        case HciStatusCode::CONNECTION_TERMINATED_MIC_FAILURE:
            return "CONNECTION_TERMINATED_MIC_FAILURE";
        case HciStatusCode::CONNECTION_FAILED_TO_BE_ESTABLISHED:
            return "CONNECTION_FAILED_TO_BE_ESTABLISHED";
        case HciStatusCode::MAC_CONNECTION_FAILED:
            return "MAC_CONNECTION_FAILED";
        default:
            return "UNKNOWN_STATUS";
    }
}

const char *hciOpcodeToString(uint16_t opcode) {
    switch (static_cast<HciOpcode>(opcode)) {
        case HciOpcode::INQUIRY:
            return "INQUIRY";
        case HciOpcode::INQUIRY_CANCEL:
            return "INQUIRY_CANCEL";
        case HciOpcode::CREATE_CONNECTION:
            return "CREATE_CONNECTION";
        case HciOpcode::REMOTE_NAME_REQUEST:
            return "REMOTE_NAME_REQUEST";
        case HciOpcode::RESET:
            return "RESET";
        case HciOpcode::WRITE_LOCAL_NAME:
            return "WRITE_LOCAL_NAME";
        case HciOpcode::WRITE_CLASS_OF_DEVICE:
            return "WRITE_CLASS_OF_DEVICE";
        case HciOpcode::WRITE_SCAN_ENABLE:
            return "WRITE_SCAN_ENABLE";
        case HciOpcode::READ_BD_ADDR:
            return "READ_BD_ADDR";
        default:
            return "UNKNOWN_OPCODE";
    }
}

const char *hciEventCodeToString(uint8_t eventCode) {
    switch (static_cast<HciEventCode>(eventCode)) {
        case HciEventCode::INQUIRY_COMPLETE:
            return "INQUIRY_COMPLETE";
        case HciEventCode::INQUIRY_RESULT:
            return "INQUIRY_RESULT";
        case HciEventCode::CONNECTION_COMPLETE:
            return "CONNECTION_COMPLETE";
        case HciEventCode::DISCONNECTION_COMPLETE:
            return "DISCONNECTION_COMPLETE";
        case HciEventCode::REMOTE_NAME_REQUEST_COMPLETE:
            return "REMOTE_NAME_REQUEST_COMPLETE";
        case HciEventCode::COMMAND_COMPLETE:
            return "COMMAND_COMPLETE";
        case HciEventCode::COMMAND_STATUS:
            return "COMMAND_STATUS";
        case HciEventCode::NUMBER_OF_COMPLETED_PACKETS:
            return "NUMBER_OF_COMPLETED_PACKETS";
        default:
            return "UNKNOWN_EVENT";
    }
}

const char *hciDisconnectionReasonToString(uint8_t reasonCode) {
    switch (static_cast<HciDisconnectionReason>(reasonCode)) {
        case HciDisconnectionReason::AUTHENTICATION_FAILURE:
            return "AUTHENTICATION_FAILURE";
        case HciDisconnectionReason::REMOTE_USER_TERMINATED_CONNECTION:
            return "REMOTE_USER_TERMINATED_CONNECTION";
        case HciDisconnectionReason::REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES";
        case HciDisconnectionReason::REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF";
        case HciDisconnectionReason::CONNECTION_TERMINATED_BY_LOCAL_HOST:
            return "CONNECTION_TERMINATED_BY_LOCAL_HOST";
        case HciDisconnectionReason::CONNECTION_TIMEOUT:
            return "CONNECTION_TIMEOUT";
        default:
            return "UNKNOWN_REASON";
    }
}
