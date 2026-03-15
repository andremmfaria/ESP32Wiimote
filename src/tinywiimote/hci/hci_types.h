// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef TINYWIIMOTE_HCI_TYPES_H
#define TINYWIIMOTE_HCI_TYPES_H

#include <stdint.h>

// HCI Events
static constexpr uint8_t HCI_INQUIRY_COMP_EVT = 0x01;
static constexpr uint8_t HCI_INQUIRY_RESULT_EVT = 0x02;
static constexpr uint8_t HCI_CONNECTION_COMP_EVT = 0x03;
static constexpr uint8_t HCI_DISCONNECTION_COMP_EVT = 0x05;
static constexpr uint8_t HCI_RMT_NAME_REQUEST_COMP_EVT = 0x07;
static constexpr uint8_t HCI_QOS_SETUP_COMP_EVT = 0x0D;
static constexpr uint8_t HCI_COMMAND_COMPLETE_EVT = 0x0E;
static constexpr uint8_t HCI_COMMAND_STATUS_EVT = 0x0F;
static constexpr uint8_t HCI_NUM_COMPL_DATA_PKTS_EVT = 0x13;

// Opcode Group Field (OGF)
static constexpr uint16_t HCI_OGF_LINK_CONTROL = 0x01;
static constexpr uint16_t HCI_OGF_CONTROL_BASEBAND = 0x03;
static constexpr uint16_t HCI_OGF_INFORMATIONAL_PARAMETERS = 0x04;

// Opcode Command Field (OCF)
static constexpr uint16_t HCI_OCF_RESET = 0x0003;
static constexpr uint16_t HCI_OCF_CHANGE_LOCAL_NAME = 0x0013;
static constexpr uint16_t HCI_OCF_WRITE_CLASS_OF_DEVICE = 0x0024;
static constexpr uint16_t HCI_OCF_WRITE_SCAN_ENABLE = 0x001A;
static constexpr uint16_t HCI_OCF_READ_BD_ADDR = 0x0009;
static constexpr uint16_t HCI_OCF_INQUIRY = 0x0001;
static constexpr uint16_t HCI_OCF_INQUIRY_CANCEL = 0x0002;
static constexpr uint16_t HCI_OCF_CREATE_CONNECTION = 0x0005;
static constexpr uint16_t HCI_OCF_REMOTE_NAME_REQUEST = 0x0019;

// HCI Command opcodes
static constexpr uint16_t HCI_OPCODE_RESET = HCI_OCF_RESET | (HCI_OGF_CONTROL_BASEBAND << 10);
static constexpr uint16_t HCI_OPCODE_WRITE_LOCAL_NAME =
    HCI_OCF_CHANGE_LOCAL_NAME | (HCI_OGF_CONTROL_BASEBAND << 10);
static constexpr uint16_t HCI_OPCODE_WRITE_CLASS_OF_DEVICE =
    HCI_OCF_WRITE_CLASS_OF_DEVICE | (HCI_OGF_CONTROL_BASEBAND << 10);
static constexpr uint16_t HCI_OPCODE_WRITE_SCAN_ENABLE =
    HCI_OCF_WRITE_SCAN_ENABLE | (HCI_OGF_CONTROL_BASEBAND << 10);
static constexpr uint16_t HCI_OPCODE_READ_BD_ADDR =
    HCI_OCF_READ_BD_ADDR | (HCI_OGF_INFORMATIONAL_PARAMETERS << 10);
static constexpr uint16_t HCI_OPCODE_INQUIRY = HCI_OCF_INQUIRY | (HCI_OGF_LINK_CONTROL << 10);
static constexpr uint16_t HCI_OPCODE_INQUIRY_CANCEL =
    HCI_OCF_INQUIRY_CANCEL | (HCI_OGF_LINK_CONTROL << 10);
static constexpr uint16_t HCI_OPCODE_CREATE_CONNECTION =
    HCI_OCF_CREATE_CONNECTION | (HCI_OGF_LINK_CONTROL << 10);
static constexpr uint16_t HCI_OPCODE_REMOTE_NAME_REQUEST =
    HCI_OCF_REMOTE_NAME_REQUEST | (HCI_OGF_LINK_CONTROL << 10);

// Command parameter sizes
static constexpr uint8_t HCIC_PARAM_SIZE_WRITE_LOCAL_NAME = 248;
static constexpr uint8_t HCIC_PARAM_SIZE_WRITE_CLASS_OF_DEVICE = 3;
static constexpr uint8_t HCIC_PARAM_SIZE_WRITE_SCAN_ENABLE = 1;
static constexpr uint8_t HCIC_PARAM_SIZE_CREATE_CONNECTION = 13;
static constexpr uint8_t HCIC_PARAM_SIZE_REMOTE_NAME_REQUEST = 10;
static constexpr uint8_t HCIC_PARAM_SIZE_WRITE_INQUIRY_CANCEL = 0;
static constexpr uint8_t HCIC_PARAM_SIZE_WRITE_INQUIRY = 5;

#endif  // TINYWIIMOTE_HCI_TYPES_H
