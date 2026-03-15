// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_commands.h"

#include "hci_types.h"

uint16_t makeCmdReset(uint8_t *buf) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_RESET);
    streamU8ToLe(buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

uint16_t makeCmdReadBdAddr(uint8_t *buf) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_READ_BD_ADDR);
    streamU8ToLe(buf, 0);
    return HCI_H4_CMD_PREAMBLE_SIZE;
}

uint16_t makeCmdWriteLocalName(uint8_t *buf, const uint8_t *name, uint8_t len) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_WRITE_LOCAL_NAME);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_WRITE_LOCAL_NAME);

    streamArray(buf, name, len);
    for (uint8_t i = len; i < HCIC_PARAM_SIZE_WRITE_LOCAL_NAME; i++) {
        streamU8ToLe(buf, 0);
    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_LOCAL_NAME;
}

uint16_t makeCmdWriteClassOfDevice(uint8_t *buf, const uint8_t *cod) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_WRITE_CLASS_OF_DEVICE);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE);

    for (uint8_t i = 0; i < HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE; i++) {
        streamU8ToLe(buf, cod[i]);
    }

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE;
}

uint16_t makeCmdWriteScanEnable(uint8_t *buf, uint8_t mode) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_WRITE_SCAN_ENABLE);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE);

    streamU8ToLe(buf, mode);
    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE;
}

uint16_t makeCmdInquiry(uint8_t *buf, const HciInquiryParams &params) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_INQUIRY);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_WRITE_INQUIRY);

    streamU8ToLe(buf, (uint8_t)(params.lap & 0xFF));
    streamU8ToLe(buf, (uint8_t)((params.lap >> 8) & 0xFF));
    streamU8ToLe(buf, (uint8_t)((params.lap >> 16) & 0xFF));
    streamU8ToLe(buf, params.length);
    streamU8ToLe(buf, params.maxResponses);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY;
}

uint16_t makeCmdInquiryCancel(uint8_t *buf) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_INQUIRY_CANCEL);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL;
}

uint16_t makeCmdRemoteNameRequest(uint8_t *buf, const HciRemoteNameRequestParams &params) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_REMOTE_NAME_REQUEST);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST);

    streamBdAddr(buf, params.bdAddr.addr);
    streamU8ToLe(buf, params.pageScanRepetitionMode);
    streamU8ToLe(buf, 0);
    streamU16ToLe(buf, params.clockOffset);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST;
}

uint16_t makeCmdCreateConnection(uint8_t *buf, const HciCreateConnectionParams &params) {
    streamU8ToLe(buf, H4TypeCommand);
    streamU16ToLe(buf, HCI_OPCODE_CREATE_CONNECTION);
    streamU8ToLe(buf, HCIC_PARAM_SIZE_CREATE_CONNECTION);

    streamBdAddr(buf, params.bdAddr.addr);
    streamU16ToLe(buf, params.packetType);
    streamU8ToLe(buf, params.pageScanRepetitionMode);
    streamU8ToLe(buf, 0);
    streamU16ToLe(buf, params.clockOffset);
    streamU8ToLe(buf, params.allowRoleSwitch);

    return HCI_H4_CMD_PREAMBLE_SIZE + HCIC_PARAM_SIZE_CREATE_CONNECTION;
}
