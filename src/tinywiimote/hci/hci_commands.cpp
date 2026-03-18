// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_commands.h"

#include "hci_types.h"

uint16_t makeCmdReset(uint8_t *buf) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeReset);
    streamU8ToLe(buf, 0);
    return kHciH4CmdPreambleSize;
}

uint16_t makeCmdReadBdAddr(uint8_t *buf) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeReadBdAddr);
    streamU8ToLe(buf, 0);
    return kHciH4CmdPreambleSize;
}

uint16_t makeCmdWriteLocalName(uint8_t *buf, const uint8_t *name, uint8_t len) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeWriteLocalName);
    streamU8ToLe(buf, kHcicParamSizeWriteLocalName);

    streamArray(buf, name, len);
    for (uint8_t i = len; i < kHcicParamSizeWriteLocalName; i++) {
        streamU8ToLe(buf, 0);
    }

    return kHciH4CmdPreambleSize + kHcicParamSizeWriteLocalName;
}

uint16_t makeCmdWriteClassOfDevice(uint8_t *buf, const uint8_t *cod) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeWriteClassOfDevice);
    streamU8ToLe(buf, kHcicParamSizeWriteClassOfDevice);

    for (uint8_t i = 0; i < kHcicParamSizeWriteClassOfDevice; i++) {
        streamU8ToLe(buf, cod[i]);
    }

    return kHciH4CmdPreambleSize + kHcicParamSizeWriteClassOfDevice;
}

uint16_t makeCmdWriteScanEnable(uint8_t *buf, uint8_t mode) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeWriteScanEnable);
    streamU8ToLe(buf, kHcicParamSizeWriteScanEnable);

    streamU8ToLe(buf, mode);
    return kHciH4CmdPreambleSize + kHcicParamSizeWriteScanEnable;
}

uint16_t makeCmdInquiry(uint8_t *buf, const HciInquiryParams &params) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeInquiry);
    streamU8ToLe(buf, kHcicParamSizeWriteInquiry);

    streamU8ToLe(buf, (uint8_t)(params.lap & 0xFF));
    streamU8ToLe(buf, (uint8_t)((params.lap >> 8) & 0xFF));
    streamU8ToLe(buf, (uint8_t)((params.lap >> 16) & 0xFF));
    streamU8ToLe(buf, params.length);
    streamU8ToLe(buf, params.maxResponses);

    return kHciH4CmdPreambleSize + kHcicParamSizeWriteInquiry;
}

uint16_t makeCmdInquiryCancel(uint8_t *buf) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeInquiryCancel);
    streamU8ToLe(buf, kHcicParamSizeWriteInquiryCancel);

    return kHciH4CmdPreambleSize + kHcicParamSizeWriteInquiryCancel;
}

uint16_t makeCmdRemoteNameRequest(uint8_t *buf, const HciRemoteNameRequestParams &params) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeRemoteNameRequest);
    streamU8ToLe(buf, kHcicParamSizeRemoteNameRequest);

    streamBdAddr(buf, params.bdAddr.addr);
    streamU8ToLe(buf, params.pageScanRepetitionMode);
    streamU8ToLe(buf, 0);
    streamU16ToLe(buf, params.clockOffset);

    return kHciH4CmdPreambleSize + kHcicParamSizeRemoteNameRequest;
}

uint16_t makeCmdCreateConnection(uint8_t *buf, const HciCreateConnectionParams &params) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeCreateConnection);
    streamU8ToLe(buf, kHcicParamSizeCreateConnection);

    streamBdAddr(buf, params.bdAddr.addr);
    streamU16ToLe(buf, params.packetType);
    streamU8ToLe(buf, params.pageScanRepetitionMode);
    streamU8ToLe(buf, 0);
    streamU16ToLe(buf, params.clockOffset);
    streamU8ToLe(buf, params.allowRoleSwitch);

    return kHciH4CmdPreambleSize + kHcicParamSizeCreateConnection;
}

uint16_t makeCmdDisconnect(uint8_t *buf, uint16_t connectionHandle, uint8_t reason) {
    streamU8ToLe(buf, static_cast<uint8_t>(H4PacketType::Command));
    streamU16ToLe(buf, kHciOpcodeDisconnect);
    streamU8ToLe(buf, kHcicParamSizeDisconnect);

    streamU16ToLe(buf, connectionHandle);
    streamU8ToLe(buf, reason);

    return kHciH4CmdPreambleSize + kHcicParamSizeDisconnect;
}
