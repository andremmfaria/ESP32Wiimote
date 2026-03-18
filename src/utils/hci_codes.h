// Copyright (c) 2026 ESP32Wiimote contributors
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_UTILS_HCI_CODES_H
#define ESP32_WIIMOTE_UTILS_HCI_CODES_H

#include <stdint.h>

enum class HciStatusCode : uint8_t {
    SUCCESS = 0x00,
    UnknownHciCommand = 0x01,
    UnknownConnectionIdentifier = 0x02,
    HardwareFailure = 0x03,
    PageTimeout = 0x04,
    AuthenticationFailure = 0x05,
    PinOrKeyMissing = 0x06,
    MemoryCapacityExceeded = 0x07,
    ConnectionTimeout = 0x08,
    ConnectionLimitExceeded = 0x09,
    SynchronousConnectionLimitExceeded = 0x0A,
    ConnectionAlreadyExists = 0x0B,
    CommandDisallowed = 0x0C,
    ConnectionRejectedLimitedResources = 0x0D,
    ConnectionRejectedSecurity = 0x0E,
    ConnectionRejectedBadBdAddr = 0x0F,
    ConnectionAcceptTimeoutExceeded = 0x10,
    UnsupportedFeatureOrParameter = 0x11,
    InvalidHciCommandParameters = 0x12,
    RemoteUserTerminatedConnection = 0x13,
    RemoteDeviceTerminatedConnectionLowResources = 0x14,
    RemoteDeviceTerminatedConnectionPowerOff = 0x15,
    ConnectionTerminatedByLocalHost = 0x16,
    RepeatedAttempts = 0x17,
    PairingNotAllowed = 0x18,
    UnknownLmpPdu = 0x19,
    UnsupportedRemoteFeature = 0x1A,
    ScoOffsetRejected = 0x1B,
    ScoIntervalRejected = 0x1C,
    ScoAirModeRejected = 0x1D,
    InvalidLmpOrLlParameters = 0x1E,
    UnspecifiedError = 0x1F,
    UnsupportedLmpOrLlParameterValue = 0x20,
    RoleChangeNotAllowed = 0x21,
    LmpOrLlResponseTimeout = 0x22,
    LmpErrorTransactionCollision = 0x23,
    LmpPduNotAllowed = 0x24,
    EncryptionModeNotAcceptable = 0x25,
    LinkKeyCannotBeChanged = 0x26,
    RequestedQosNotSupported = 0x27,
    InstantPassed = 0x28,
    PairingWithUnitKeyNotSupported = 0x29,
    DifferentTransactionCollision = 0x2A,
    QosUnacceptableParameter = 0x2C,
    QosRejected = 0x2D,
    ChannelClassificationNotSupported = 0x2E,
    InsufficientSecurity = 0x2F,
    ParameterOutOfMandatoryRange = 0x30,
    RoleSwitchPending = 0x32,
    ReservedSlotViolation = 0x34,
    RoleSwitchFailed = 0x35,
    ExtendedInquiryResponseTooLarge = 0x36,
    SecureSimplePairingNotSupportedByHost = 0x37,
    HostBusyPairing = 0x38,
    ConnectionRejectedNoSuitableChannelFound = 0x39,
    ControllerBusy = 0x3A,
    UnacceptableConnectionParameters = 0x3B,
    AdvertisingTimeout = 0x3C,
    ConnectionTerminatedMicFailure = 0x3D,
    ConnectionFailedToBeEstablished = 0x3E,
    MacConnectionFailed = 0x3F,
};

enum class HciOpcode : uint16_t {
    INQUIRY = 0x0401,
    InquiryCancel = 0x0402,
    CreateConnection = 0x0405,
    Disconnect = 0x0406,
    RemoteNameRequest = 0x0419,

    RESET = 0x0C03,
    WriteLocalName = 0x0C13,
    WriteClassOfDevice = 0x0C24,
    WriteScanEnable = 0x0C1A,

    ReadBdAddr = 0x1009,
};

enum class HciEventCode : uint8_t {
    InquiryComplete = 0x01,
    InquiryResult = 0x02,
    ConnectionComplete = 0x03,
    DisconnectionComplete = 0x05,
    RemoteNameRequestComplete = 0x07,
    CommandComplete = 0x0E,
    CommandStatus = 0x0F,
    NumberOfCompletedPackets = 0x13,
};

enum class HciDisconnectionReason : uint8_t {
    AuthenticationFailure = 0x05,
    RemoteUserTerminatedConnection = 0x13,
    RemoteDeviceTerminatedConnectionLowResources = 0x14,
    RemoteDeviceTerminatedConnectionPowerOff = 0x15,
    ConnectionTerminatedByLocalHost = 0x16,
    ConnectionTimeout = 0x08,
};

const char *hciStatusCodeToString(uint8_t statusCode);
const char *hciOpcodeToString(uint16_t opcode);
const char *hciEventCodeToString(uint8_t eventCode);
const char *hciDisconnectionReasonToString(uint8_t reasonCode);

#endif  // ESP32_WIIMOTE_UTILS_HCI_CODES_H
