// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_commands.h"

#include "hci_types.h"

uint16_t make_cmd_reset(uint8_t *buf) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_RESET);
    UINT8_TO_STREAM(buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

uint16_t make_cmd_read_bd_addr(uint8_t *buf) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_READ_BD_ADDR);
    UINT8_TO_STREAM(buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

uint16_t make_cmd_write_local_name(uint8_t *buf, const uint8_t *name, uint8_t len) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_WRITE_LOCAL_NAME);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_LOCAL_NAME);

    ARRAY_TO_STREAM(buf, name, len);
    for (uint8_t i = len; i < HCIC_PARAM_SIZE_WRITE_LOCAL_NAME; i++) {
        UINT8_TO_STREAM(buf, 0);
    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_LOCAL_NAME;
}

uint16_t make_cmd_write_class_of_device(uint8_t *buf, const uint8_t *cod) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_WRITE_CLASS_OF_DEVICE);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE);

    for (uint8_t i = 0; i < HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE; i++) {
        UINT8_TO_STREAM(buf, cod[i]);
    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE;
}

uint16_t make_cmd_write_scan_enable(uint8_t *buf, uint8_t mode) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_WRITE_SCAN_ENABLE);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE);

    UINT8_TO_STREAM(buf, mode);
    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE;
}

uint16_t make_cmd_inquiry(uint8_t *buf, uint32_t lap, uint8_t len, uint8_t num) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_INQUIRY);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_INQUIRY);

    UINT8_TO_STREAM(buf, (uint8_t)(lap & 0xFF));
    UINT8_TO_STREAM(buf, (uint8_t)((lap >> 8) & 0xFF));
    UINT8_TO_STREAM(buf, (uint8_t)((lap >> 16) & 0xFF));
    UINT8_TO_STREAM(buf, len);
    UINT8_TO_STREAM(buf, num);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY;
}

uint16_t make_cmd_inquiry_cancel(uint8_t *buf) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_INQUIRY_CANCEL);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL;
}

uint16_t make_cmd_remote_name_request(uint8_t *buf,
                                      struct bd_addr_t bdAddr,
                                      uint8_t psrm,
                                      uint16_t clkofs) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_REMOTE_NAME_REQUEST);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST);

    BDADDR_TO_STREAM(buf, bdAddr.addr);
    UINT8_TO_STREAM(buf, psrm);
    UINT8_TO_STREAM(buf, 0);
    UINT16_TO_STREAM(buf, clkofs);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST;
}

uint16_t make_cmd_create_connection(uint8_t *buf,
                                    struct bd_addr_t bdAddr,
                                    uint16_t pt,
                                    uint8_t psrm,
                                    uint16_t clkofs,
                                    uint8_t ars) {
    UINT8_TO_STREAM(buf, H4_TYPE_COMMAND);
    UINT16_TO_STREAM(buf, HCI_OPCODE_CREATE_CONNECTION);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_CREATE_CONNECTION);

    BDADDR_TO_STREAM(buf, bdAddr.addr);
    UINT16_TO_STREAM(buf, pt);
    UINT8_TO_STREAM(buf, psrm);
    UINT8_TO_STREAM(buf, 0);
    UINT16_TO_STREAM(buf, clkofs);
    UINT8_TO_STREAM(buf, ars);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CREATE_CONNECTION;
}
