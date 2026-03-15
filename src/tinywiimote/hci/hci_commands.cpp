// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_commands.h"

#include "hci_types.h"

uint16_t makeCmdReset(uint8_t *buf) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_RESET);
    UINT8_TO_STREAM(buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

uint16_t makeCmdReadBdAddr(uint8_t *buf) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_READ_BD_ADDR);
    UINT8_TO_STREAM(buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

uint16_t makeCmdWriteLocalName(uint8_t *buf, const uint8_t *name, uint8_t len) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_WRITE_LOCAL_NAME);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_LOCAL_NAME);

    ARRAY_TO_STREAM(buf, name, len);
    for (uint8_t i = len; i < HCIC_PARAM_SIZE_WRITE_LOCAL_NAME; i++) {
        UINT8_TO_STREAM(buf, 0);
    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_LOCAL_NAME;
}

uint16_t makeCmdWriteClassOfDevice(uint8_t *buf, const uint8_t *cod) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_WRITE_CLASS_OF_DEVICE);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE);

    for (uint8_t i = 0; i < HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE; i++) {
        UINT8_TO_STREAM(buf, cod[i]);
    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE;
}

uint16_t makeCmdWriteScanEnable(uint8_t *buf, uint8_t mode) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_WRITE_SCAN_ENABLE);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE);

    UINT8_TO_STREAM(buf, mode);
    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE;
}

uint16_t makeCmdInquiry(uint8_t *buf, const HciInquiryParams &params) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_INQUIRY);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_INQUIRY);

    UINT8_TO_STREAM(buf, (uint8_t)(params.lap & 0xFF));
    UINT8_TO_STREAM(buf, (uint8_t)((params.lap >> 8) & 0xFF));
    UINT8_TO_STREAM(buf, (uint8_t)((params.lap >> 16) & 0xFF));
    UINT8_TO_STREAM(buf, params.length);
    UINT8_TO_STREAM(buf, params.maxResponses);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY;
}

uint16_t makeCmdInquiryCancel(uint8_t *buf) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_INQUIRY_CANCEL);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL;
}

uint16_t makeCmdRemoteNameRequest(uint8_t *buf, const HciRemoteNameRequestParams &params) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_REMOTE_NAME_REQUEST);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST);

    BDADDR_TO_STREAM(buf, params.bdAddr.addr);
    UINT8_TO_STREAM(buf, params.pageScanRepetitionMode);
    UINT8_TO_STREAM(buf, 0);
    UINT16_TO_STREAM(buf, params.clockOffset);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST;
}

uint16_t makeCmdCreateConnection(uint8_t *buf, const HciCreateConnectionParams &params) {
    UINT8_TO_STREAM(buf, H4TypeCommand);
    UINT16_TO_STREAM(buf, HCI_OPCODE_CREATE_CONNECTION);
    UINT8_TO_STREAM(buf, HCIC_PARAM_SIZE_CREATE_CONNECTION);

    BDADDR_TO_STREAM(buf, params.bdAddr.addr);
    UINT16_TO_STREAM(buf, params.packetType);
    UINT8_TO_STREAM(buf, params.pageScanRepetitionMode);
    UINT8_TO_STREAM(buf, 0);
    UINT16_TO_STREAM(buf, params.clockOffset);
    UINT8_TO_STREAM(buf, params.allowRoleSwitch);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CREATE_CONNECTION;
}
