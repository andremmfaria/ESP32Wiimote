// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32WIIMOTE_TINYWIIMOTE_HCI_COMMANDS_H_
#define ESP32WIIMOTE_TINYWIIMOTE_HCI_COMMANDS_H_

#include "../utils/hci_utils.h"

#include <stdint.h>

struct HciInquiryParams {
    uint32_t lap;
    uint8_t length;
    uint8_t maxResponses;
};

struct HciRemoteNameRequestParams {
    struct bd_addr_t bdAddr;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
};

struct HciCreateConnectionParams {
    struct bd_addr_t bdAddr;
    uint16_t packetType;
    uint8_t pageScanRepetitionMode;
    uint16_t clockOffset;
    uint8_t allowRoleSwitch;
};

uint16_t make_cmd_reset(uint8_t *buf);
uint16_t make_cmd_read_bd_addr(uint8_t *buf);
uint16_t make_cmd_write_local_name(uint8_t *buf, const uint8_t *name, uint8_t len);
uint16_t make_cmd_write_class_of_device(uint8_t *buf, const uint8_t *cod);
uint16_t make_cmd_write_scan_enable(uint8_t *buf, uint8_t mode);
uint16_t make_cmd_inquiry(uint8_t *buf, const HciInquiryParams &params);
uint16_t make_cmd_inquiry_cancel(uint8_t *buf);
uint16_t make_cmd_remote_name_request(uint8_t *buf, const HciRemoteNameRequestParams &params);
uint16_t make_cmd_create_connection(uint8_t *buf, const HciCreateConnectionParams &params);

#endif  // ESP32WIIMOTE_TINYWIIMOTE_HCI_COMMANDS_H_
