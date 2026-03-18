// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_TINYWIIMOTE_HCI_COMMANDS_H
#define ESP32_WIIMOTE_TINYWIIMOTE_HCI_COMMANDS_H

#include "../utils/hci_utils.h"

#include <stdint.h>

struct HciInquiryParams {
    uint32_t lap;
    uint8_t length;
    uint8_t maxResponses;
};

struct HciRemoteNameRequestParams {
    struct BdAddrT bdAddr;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
};

struct HciCreateConnectionParams {
    struct BdAddrT bdAddr;
    uint16_t packetType;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
    uint8_t allowRoleSwitch;
};

uint16_t makeCmdReset(uint8_t *buf);
uint16_t makeCmdReadBdAddr(uint8_t *buf);
uint16_t makeCmdWriteLocalName(uint8_t *buf, const uint8_t *name, uint8_t len);
uint16_t makeCmdWriteClassOfDevice(uint8_t *buf, const uint8_t *cod);
uint16_t makeCmdWriteScanEnable(uint8_t *buf, uint8_t mode);
uint16_t makeCmdInquiry(uint8_t *buf, const HciInquiryParams &params);
uint16_t makeCmdInquiryCancel(uint8_t *buf);
uint16_t makeCmdRemoteNameRequest(uint8_t *buf, const HciRemoteNameRequestParams &params);
uint16_t makeCmdCreateConnection(uint8_t *buf, const HciCreateConnectionParams &params);
uint16_t makeCmdDisconnect(uint8_t *buf, uint16_t connectionHandle, uint8_t reason);

#endif  // ESP32_WIIMOTE_TINYWIIMOTE_HCI_COMMANDS_H
