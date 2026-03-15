// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_events.h"

#include "../../utils/hci_codes.h"
#include "../../utils/serial_logging.h"
#include "hci_commands.h"
#include "hci_types.h"

#include <string.h>

static uint8_t gHciTxBuffer[256];

static void clearScannedDevices(struct HciEventContext *ctx) {
    ctx->scannedDeviceCount = 0;
}

static int findScannedDevice(struct HciEventContext *ctx, struct BdAddrT bdAddr) {
    for (int i = 0; i < ctx->scannedDeviceCount; i++) {
        if (memcmp(ctx->scannedDevices[i].bdAddr.addr, bdAddr.addr, BD_ADDR_LEN) == 0) {
            return i;
        }
    }
    return -1;
}

static int addScannedDevice(struct HciEventContext *ctx, struct HciScannedDevice scanned) {
    if (ctx->scannedDeviceCount >= HCI_SCANNED_DEVICE_LIST_SIZE) {
        return -1;
    }
    ctx->scannedDevices[ctx->scannedDeviceCount++] = scanned;
    return ctx->scannedDeviceCount;
}

static void hciSend(struct HciEventContext *ctx, uint16_t len) {
    if (ctx->sendPacket != nullptr) {
        ctx->sendPacket(gHciTxBuffer, len, ctx->userData);
    }
}

void hciEventsInit(struct HciEventContext *ctx, HciSendPacketFunc sendPacket, void *userData) {
    if (ctx == nullptr) {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->sendPacket = sendPacket;
    ctx->userData = userData;
}

void hciEventsSetCallbacks(struct HciEventContext *ctx,
                           HciAclConnectedFunc onAclConnected,
                           HciDisconnectedFunc onDisconnected) {
    if (ctx == nullptr) {
        return;
    }

    ctx->onAclConnected = onAclConnected;
    ctx->onDisconnected = onDisconnected;
}

void hciEventsResetDevice(struct HciEventContext *ctx) {
    if (ctx == nullptr) {
        return;
    }

    clearScannedDevices(ctx);
    const uint16_t kTxLen = makeCmdReset(gHciTxBuffer);
    hciSend(ctx, kTxLen);
}

static void handleCommandComplete(struct HciEventContext *ctx, const uint8_t *data) {
    const uint16_t kCmdOpcode = readUinT16Le(data + 1);
    const uint8_t kStatus = data[3];

    switch (kCmdOpcode) {
        case HCI_OPCODE_RESET: {
            if (kStatus == 0x00) {
                LOG_DEBUG("HCI: Reset successful\n");
                const uint16_t kTxLen = makeCmdReadBdAddr(gHciTxBuffer);
                hciSend(ctx, kTxLen);
            } else {
                LOG_ERROR("HCI: %s (0x%04x) failed, status=0x%02x (%s)\n",
                          hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                          hciStatusCodeToString(kStatus));
            }
            break;
        }

        case HCI_OPCODE_READ_BD_ADDR: {
            if (kStatus == 0x00) {
                static const uint8_t kName[] = "ESP32-BT-L2CAP";
                const uint16_t kTxLen = makeCmdWriteLocalName(gHciTxBuffer, kName, sizeof(kName));
                hciSend(ctx, kTxLen);
            } else {
                LOG_ERROR("HCI: %s (0x%04x) failed, status=0x%02x (%s)\n",
                          hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                          hciStatusCodeToString(kStatus));
            }
            break;
        }

        case HCI_OPCODE_WRITE_LOCAL_NAME: {
            if (kStatus == 0x00) {
                static const uint8_t kClassOfDevice[3] = {0x04, 0x05, 0x00};
                const uint16_t kTxLen = makeCmdWriteClassOfDevice(gHciTxBuffer, kClassOfDevice);
                hciSend(ctx, kTxLen);
            } else {
                LOG_ERROR("HCI: %s (0x%04x) failed, status=0x%02x (%s)\n",
                          hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                          hciStatusCodeToString(kStatus));
            }
            break;
        }

        case HCI_OPCODE_WRITE_CLASS_OF_DEVICE: {
            if (kStatus == 0x00) {
                const uint16_t kTxLen = makeCmdWriteScanEnable(gHciTxBuffer, 3);
                hciSend(ctx, kTxLen);
            } else {
                LOG_ERROR("HCI: %s (0x%04x) failed, status=0x%02x (%s)\n",
                          hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                          hciStatusCodeToString(kStatus));
            }
            break;
        }

        case HCI_OPCODE_WRITE_SCAN_ENABLE: {
            if (kStatus == 0x00) {
                LOG_INFO("HCI: Device initialized, starting inquiry\n");
                clearScannedDevices(ctx);
                HciInquiryParams inquiryParams = {0x9E8B33, 0x05, 0x00};
                const uint16_t kTxLen = makeCmdInquiry(gHciTxBuffer, inquiryParams);
                hciSend(ctx, kTxLen);
                ctx->deviceInited = true;
            } else {
                LOG_ERROR("HCI: %s (0x%04x) failed, status=0x%02x (%s)\n",
                          hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                          hciStatusCodeToString(kStatus));
            }
            break;
        }

        case HCI_OPCODE_INQUIRY_CANCEL:
            break;

        default:
            if (kStatus != 0x00) {
                LOG_WARN("HCI: %s (0x%04x) completed with error status=0x%02x (%s)\n",
                         hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                         hciStatusCodeToString(kStatus));
            }
            break;
    }
}

static void handleCommandStatus(struct HciEventContext *ctx, const uint8_t *data) {
    (void)ctx;

    const uint8_t kStatus = data[0];
    const uint8_t kNumHciCommandPackets = data[1];
    const uint16_t kCmdOpcode = readUinT16Le(data + 2);

    if (kStatus != 0x00) {
        LOG_WARN("HCI: Command status for %s (0x%04x): status=0x%02x (%s), numPackets=%u\n",
                 hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus, hciStatusCodeToString(kStatus),
                 kNumHciCommandPackets);
    }
}

static void handleInquiryComplete(struct HciEventContext *ctx, const uint8_t *data) {
    (void)data;
    hciEventsResetDevice(ctx);
}

static void handleInquiryResult(struct HciEventContext *ctx, const uint8_t *data) {
    const uint8_t kNum = data[0];
    LOG_DEBUG("HCI: Inquiry result: found %d device(s)\n", kNum);

    for (int i = 0; i < kNum; i++) {
        const int kPos = 1 + ((6 + 1 + 2 + 3 + 2) * i);

        struct BdAddrT bdAddr;
        STREAM_TO_BDADDR(bdAddr.addr, data + kPos);

        int idx = findScannedDevice(ctx, bdAddr);
        if (idx != -1) {
            continue;
        }

        struct HciScannedDevice scanned;
        scanned.bdAddr = bdAddr;
        scanned.psrm = data[kPos + 6];
        scanned.clkofs = ((0x80 | data[kPos + 12]) << 8) | (data[kPos + 13]);

        idx = addScannedDevice(ctx, scanned);
        if (idx < 0) {
            continue;
        }

        // Filter for Wiimote class-of-device: [04 25 00]
        if (data[kPos + 9] == 0x04 && data[kPos + 10] == 0x25 && data[kPos + 11] == 0x00) {
            LOG_INFO("HCI: Wiimote detected! Requesting remote name...\n");
            HciRemoteNameRequestParams remoteNameParams = {
                scanned.bdAddr,
                scanned.psrm,
                scanned.clkofs,
            };
            const uint16_t kTxLen = makeCmdRemoteNameRequest(gHciTxBuffer, remoteNameParams);
            hciSend(ctx, kTxLen);
        }
    }
}

static void handleRemoteNameRequestComplete(struct HciEventContext *ctx, uint8_t *data) {
    struct BdAddrT bdAddr;
    STREAM_TO_BDADDR(bdAddr.addr, data + 1);

    char *name = (char *)(data + 7);

    LOG_DEBUG("HCI: Remote name: %s\n", name);

    const int kIdx = findScannedDevice(ctx, bdAddr);
    if (kIdx < 0) {
        return;
    }

    if (strcmp("Nintendo RVL-CNT-01", name) == 0) {
        LOG_INFO("HCI: Nintendo Wiimote confirmed! Initiating connection...\n");
        const uint16_t kCancelLen = makeCmdInquiryCancel(gHciTxBuffer);
        hciSend(ctx, kCancelLen);

        const struct HciScannedDevice kScanned = ctx->scannedDevices[kIdx];
        const uint16_t kPacketType = 0x0008;
        const uint8_t kAllowRoleSwitch = 0x00;

        HciCreateConnectionParams createConnectionParams = {
            kScanned.bdAddr, kPacketType, kScanned.psrm, kScanned.clkofs, kAllowRoleSwitch,
        };
        const uint16_t kTxLen = makeCmdCreateConnection(gHciTxBuffer, createConnectionParams);
        hciSend(ctx, kTxLen);
    }
}

static void handleConnectionComplete(struct HciEventContext *ctx, uint8_t *data) {
    const uint16_t kConnectionHandle = readUinT16Le(data + 1);
    LOG_INFO("HCI: Connection complete! Handle: 0x%04x\n", kConnectionHandle);
    if (ctx->onAclConnected != nullptr) {
        ctx->onAclConnected(kConnectionHandle, ctx->userData);
    }
}

static void handleDisconnectionComplete(struct HciEventContext *ctx, const uint8_t *data) {
    const uint16_t kConnectionHandle = readUinT16Le(data + 1);
    const uint8_t kReason = data[3];

    LOG_INFO("HCI: Disconnection complete! Handle: 0x%04x, Reason: 0x%02x (%s)\n",
             kConnectionHandle, kReason, hciDisconnectionReasonToString(kReason));

    if (ctx->onDisconnected != nullptr) {
        ctx->onDisconnected(kConnectionHandle, kReason, ctx->userData);
    }
}

void hciEventsHandleEvent(struct HciEventContext *ctx, const HciEventPacket &packet) {
    (void)packet.len;

    if (ctx == nullptr || packet.data == nullptr) {
        return;
    }

    switch (packet.eventCode) {
        case HCI_INQUIRY_COMP_EVT:
            handleInquiryComplete(ctx, packet.data);
            break;

        case HCI_INQUIRY_RESULT_EVT:
            handleInquiryResult(ctx, packet.data);
            break;

        case HCI_CONNECTION_COMP_EVT:
            handleConnectionComplete(ctx, packet.data);
            break;

        case HCI_DISCONNECTION_COMP_EVT:
            handleDisconnectionComplete(ctx, packet.data);
            break;

        case HCI_RMT_NAME_REQUEST_COMP_EVT:
            handleRemoteNameRequestComplete(ctx, packet.data);
            break;

        case HCI_COMMAND_COMPLETE_EVT:
            handleCommandComplete(ctx, packet.data);
            break;

        case HCI_COMMAND_STATUS_EVT:
            handleCommandStatus(ctx, packet.data);
            break;

        default:
            LOG_DEBUG("HCI: Unhandled event 0x%02x (%s)\n", packet.eventCode,
                      hciEventCodeToString(packet.eventCode));
            break;
    }
}
