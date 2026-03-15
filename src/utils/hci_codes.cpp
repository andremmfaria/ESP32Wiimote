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
        case HciStatusCode::UnknownHciCommand:
            return "UNKNOWN_HCI_COMMAND";
        case HciStatusCode::UnknownConnectionIdentifier:
            return "UNKNOWN_CONNECTION_IDENTIFIER";
        case HciStatusCode::HardwareFailure:
            return "HARDWARE_FAILURE";
        case HciStatusCode::PageTimeout:
            return "PAGE_TIMEOUT";
        case HciStatusCode::AuthenticationFailure:
            return "AUTHENTICATION_FAILURE";
        case HciStatusCode::PinOrKeyMissing:
            return "PIN_OR_KEY_MISSING";
        case HciStatusCode::MemoryCapacityExceeded:
            return "MEMORY_CAPACITY_EXCEEDED";
        case HciStatusCode::ConnectionTimeout:
            return "CONNECTION_TIMEOUT";
        case HciStatusCode::ConnectionLimitExceeded:
            return "CONNECTION_LIMIT_EXCEEDED";
        case HciStatusCode::SynchronousConnectionLimitExceeded:
            return "SYNCHRONOUS_CONNECTION_LIMIT_EXCEEDED";
        case HciStatusCode::ConnectionAlreadyExists:
            return "CONNECTION_ALREADY_EXISTS";
        case HciStatusCode::CommandDisallowed:
            return "COMMAND_DISALLOWED";
        case HciStatusCode::ConnectionRejectedLimitedResources:
            return "CONNECTION_REJECTED_LIMITED_RESOURCES";
        case HciStatusCode::ConnectionRejectedSecurity:
            return "CONNECTION_REJECTED_SECURITY";
        case HciStatusCode::ConnectionRejectedBadBdAddr:
            return "CONNECTION_REJECTED_BAD_BD_ADDR";
        case HciStatusCode::ConnectionAcceptTimeoutExceeded:
            return "CONNECTION_ACCEPT_TIMEOUT_EXCEEDED";
        case HciStatusCode::UnsupportedFeatureOrParameter:
            return "UNSUPPORTED_FEATURE_OR_PARAMETER";
        case HciStatusCode::InvalidHciCommandParameters:
            return "INVALID_HCI_COMMAND_PARAMETERS";
        case HciStatusCode::RemoteUserTerminatedConnection:
            return "REMOTE_USER_TERMINATED_CONNECTION";
        case HciStatusCode::RemoteDeviceTerminatedConnectionLowResources:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES";
        case HciStatusCode::RemoteDeviceTerminatedConnectionPowerOff:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF";
        case HciStatusCode::ConnectionTerminatedByLocalHost:
            return "CONNECTION_TERMINATED_BY_LOCAL_HOST";
        case HciStatusCode::RepeatedAttempts:
            return "REPEATED_ATTEMPTS";
        case HciStatusCode::PairingNotAllowed:
            return "PAIRING_NOT_ALLOWED";
        case HciStatusCode::UnknownLmpPdu:
            return "UNKNOWN_LMP_PDU";
        case HciStatusCode::UnsupportedRemoteFeature:
            return "UNSUPPORTED_REMOTE_FEATURE";
        case HciStatusCode::ScoOffsetRejected:
            return "SCO_OFFSET_REJECTED";
        case HciStatusCode::ScoIntervalRejected:
            return "SCO_INTERVAL_REJECTED";
        case HciStatusCode::ScoAirModeRejected:
            return "SCO_AIR_MODE_REJECTED";
        case HciStatusCode::InvalidLmpOrLlParameters:
            return "INVALID_LMP_OR_LL_PARAMETERS";
        case HciStatusCode::UnspecifiedError:
            return "UNSPECIFIED_ERROR";
        case HciStatusCode::UnsupportedLmpOrLlParameterValue:
            return "UNSUPPORTED_LMP_OR_LL_PARAMETER_VALUE";
        case HciStatusCode::RoleChangeNotAllowed:
            return "ROLE_CHANGE_NOT_ALLOWED";
        case HciStatusCode::LmpOrLlResponseTimeout:
            return "LMP_OR_LL_RESPONSE_TIMEOUT";
        case HciStatusCode::LmpErrorTransactionCollision:
            return "LMP_ERROR_TRANSACTION_COLLISION";
        case HciStatusCode::LmpPduNotAllowed:
            return "LMP_PDU_NOT_ALLOWED";
        case HciStatusCode::EncryptionModeNotAcceptable:
            return "ENCRYPTION_MODE_NOT_ACCEPTABLE";
        case HciStatusCode::LinkKeyCannotBeChanged:
            return "LINK_KEY_CANNOT_BE_CHANGED";
        case HciStatusCode::RequestedQosNotSupported:
            return "REQUESTED_QOS_NOT_SUPPORTED";
        case HciStatusCode::InstantPassed:
            return "INSTANT_PASSED";
        case HciStatusCode::PairingWithUnitKeyNotSupported:
            return "PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED";
        case HciStatusCode::DifferentTransactionCollision:
            return "DIFFERENT_TRANSACTION_COLLISION";
        case HciStatusCode::QosUnacceptableParameter:
            return "QOS_UNACCEPTABLE_PARAMETER";
        case HciStatusCode::QosRejected:
            return "QOS_REJECTED";
        case HciStatusCode::ChannelClassificationNotSupported:
            return "CHANNEL_CLASSIFICATION_NOT_SUPPORTED";
        case HciStatusCode::InsufficientSecurity:
            return "INSUFFICIENT_SECURITY";
        case HciStatusCode::ParameterOutOfMandatoryRange:
            return "PARAMETER_OUT_OF_MANDATORY_RANGE";
        case HciStatusCode::RoleSwitchPending:
            return "ROLE_SWITCH_PENDING";
        case HciStatusCode::ReservedSlotViolation:
            return "RESERVED_SLOT_VIOLATION";
        case HciStatusCode::RoleSwitchFailed:
            return "ROLE_SWITCH_FAILED";
        case HciStatusCode::ExtendedInquiryResponseTooLarge:
            return "EXTENDED_INQUIRY_RESPONSE_TOO_LARGE";
        case HciStatusCode::SecureSimplePairingNotSupportedByHost:
            return "SECURE_SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST";
        case HciStatusCode::HostBusyPairing:
            return "HOST_BUSY_PAIRING";
        case HciStatusCode::ConnectionRejectedNoSuitableChannelFound:
            return "CONNECTION_REJECTED_NO_SUITABLE_CHANNEL_FOUND";
        case HciStatusCode::ControllerBusy:
            return "CONTROLLER_BUSY";
        case HciStatusCode::UnacceptableConnectionParameters:
            return "UNACCEPTABLE_CONNECTION_PARAMETERS";
        case HciStatusCode::AdvertisingTimeout:
            return "ADVERTISING_TIMEOUT";
        case HciStatusCode::ConnectionTerminatedMicFailure:
            return "CONNECTION_TERMINATED_MIC_FAILURE";
        case HciStatusCode::ConnectionFailedToBeEstablished:
            return "CONNECTION_FAILED_TO_BE_ESTABLISHED";
        case HciStatusCode::MacConnectionFailed:
            return "MAC_CONNECTION_FAILED";
        default:
            return "UNKNOWN_STATUS";
    }
}

const char *hciOpcodeToString(uint16_t opcode) {
    switch (static_cast<HciOpcode>(opcode)) {
        case HciOpcode::INQUIRY:
            return "INQUIRY";
        case HciOpcode::InquiryCancel:
            return "INQUIRY_CANCEL";
        case HciOpcode::CreateConnection:
            return "CREATE_CONNECTION";
        case HciOpcode::RemoteNameRequest:
            return "REMOTE_NAME_REQUEST";
        case HciOpcode::RESET:
            return "RESET";
        case HciOpcode::WriteLocalName:
            return "WRITE_LOCAL_NAME";
        case HciOpcode::WriteClassOfDevice:
            return "WRITE_CLASS_OF_DEVICE";
        case HciOpcode::WriteScanEnable:
            return "WRITE_SCAN_ENABLE";
        case HciOpcode::ReadBdAddr:
            return "READ_BD_ADDR";
        default:
            return "UNKNOWN_OPCODE";
    }
}

const char *hciEventCodeToString(uint8_t eventCode) {
    switch (static_cast<HciEventCode>(eventCode)) {
        case HciEventCode::InquiryComplete:
            return "INQUIRY_COMPLETE";
        case HciEventCode::InquiryResult:
            return "INQUIRY_RESULT";
        case HciEventCode::ConnectionComplete:
            return "CONNECTION_COMPLETE";
        case HciEventCode::DisconnectionComplete:
            return "DISCONNECTION_COMPLETE";
        case HciEventCode::RemoteNameRequestComplete:
            return "REMOTE_NAME_REQUEST_COMPLETE";
        case HciEventCode::CommandComplete:
            return "COMMAND_COMPLETE";
        case HciEventCode::CommandStatus:
            return "COMMAND_STATUS";
        case HciEventCode::NumberOfCompletedPackets:
            return "NUMBER_OF_COMPLETED_PACKETS";
        default:
            return "UNKNOWN_EVENT";
    }
}

const char *hciDisconnectionReasonToString(uint8_t reasonCode) {
    switch (static_cast<HciDisconnectionReason>(reasonCode)) {
        case HciDisconnectionReason::AuthenticationFailure:
            return "AUTHENTICATION_FAILURE";
        case HciDisconnectionReason::RemoteUserTerminatedConnection:
            return "REMOTE_USER_TERMINATED_CONNECTION";
        case HciDisconnectionReason::RemoteDeviceTerminatedConnectionLowResources:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_LOW_RESOURCES";
        case HciDisconnectionReason::RemoteDeviceTerminatedConnectionPowerOff:
            return "REMOTE_DEVICE_TERMINATED_CONNECTION_POWER_OFF";
        case HciDisconnectionReason::ConnectionTerminatedByLocalHost:
            return "CONNECTION_TERMINATED_BY_LOCAL_HOST";
        case HciDisconnectionReason::ConnectionTimeout:
            return "CONNECTION_TIMEOUT";
        default:
            return "UNKNOWN_REASON";
    }
}
