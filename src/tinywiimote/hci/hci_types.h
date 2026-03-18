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
static constexpr uint8_t kHciInquiryCompEvt = 0x01;
static constexpr uint8_t kHciInquiryResultEvt = 0x02;
static constexpr uint8_t kHciConnectionCompEvt = 0x03;
static constexpr uint8_t kHciDisconnectionCompEvt = 0x05;
static constexpr uint8_t kHciRmtNameRequestCompEvt = 0x07;
static constexpr uint8_t kHciQosSetupCompEvt = 0x0D;
static constexpr uint8_t kHciCommandCompleteEvt = 0x0E;
static constexpr uint8_t kHciCommandStatusEvt = 0x0F;
static constexpr uint8_t kHciNumComplDataPktsEvt = 0x13;

// Opcode Group Field (OGF)
static constexpr uint16_t kHciOgfLinkControl = 0x01;
static constexpr uint16_t kHciOgfControlBaseband = 0x03;
static constexpr uint16_t kHciOgfInformationalParameters = 0x04;

// Opcode Command Field (OCF)
static constexpr uint16_t kHciOcfReset = 0x0003;
static constexpr uint16_t kHciOcfChangeLocalName = 0x0013;
static constexpr uint16_t kHciOcfWriteClassOfDevice = 0x0024;
static constexpr uint16_t kHciOcfWriteScanEnable = 0x001A;
static constexpr uint16_t kHciOcfReadBdAddr = 0x0009;
static constexpr uint16_t kHciOcfInquiry = 0x0001;
static constexpr uint16_t kHciOcfInquiryCancel = 0x0002;
static constexpr uint16_t kHciOcfCreateConnection = 0x0005;
static constexpr uint16_t kHciOcfDisconnect = 0x0006;
static constexpr uint16_t kHciOcfRemoteNameRequest = 0x0019;

// HCI Command opcodes
static constexpr uint16_t kHciOpcodeReset = kHciOcfReset | (kHciOgfControlBaseband << 10);
static constexpr uint16_t kHciOpcodeWriteLocalName =
    kHciOcfChangeLocalName | (kHciOgfControlBaseband << 10);
static constexpr uint16_t kHciOpcodeWriteClassOfDevice =
    kHciOcfWriteClassOfDevice | (kHciOgfControlBaseband << 10);
static constexpr uint16_t kHciOpcodeWriteScanEnable =
    kHciOcfWriteScanEnable | (kHciOgfControlBaseband << 10);
static constexpr uint16_t kHciOpcodeReadBdAddr =
    kHciOcfReadBdAddr | (kHciOgfInformationalParameters << 10);
static constexpr uint16_t kHciOpcodeInquiry = kHciOcfInquiry | (kHciOgfLinkControl << 10);
static constexpr uint16_t kHciOpcodeInquiryCancel =
    kHciOcfInquiryCancel | (kHciOgfLinkControl << 10);
static constexpr uint16_t kHciOpcodeCreateConnection =
    kHciOcfCreateConnection | (kHciOgfLinkControl << 10);
static constexpr uint16_t kHciOpcodeDisconnect = kHciOcfDisconnect | (kHciOgfLinkControl << 10);
static constexpr uint16_t kHciOpcodeRemoteNameRequest =
    kHciOcfRemoteNameRequest | (kHciOgfLinkControl << 10);

// Command parameter sizes
static constexpr uint8_t kHcicParamSizeWriteLocalName = 248;
static constexpr uint8_t kHcicParamSizeWriteClassOfDevice = 3;
static constexpr uint8_t kHcicParamSizeWriteScanEnable = 1;
static constexpr uint8_t kHcicParamSizeCreateConnection = 13;
static constexpr uint8_t kHcicParamSizeDisconnect = 3;
static constexpr uint8_t kHcicParamSizeRemoteNameRequest = 10;
static constexpr uint8_t kHcicParamSizeWriteInquiryCancel = 0;
static constexpr uint8_t kHcicParamSizeWriteInquiry = 5;

#endif  // TINYWIIMOTE_HCI_TYPES_H
