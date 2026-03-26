// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_events.h"

#include "../../utils/hci_codes.h"
#include "../../utils/serial_logging.h"
#include "Arduino.h"
#include "hci_commands.h"
#include "hci_types.h"

#include <string.h>

static uint8_t gHciTxBuffer[256];
static const uint32_t kFastReconnectTtlMs = 3UL * 60UL * 1000UL;

static void clearScannedDevices(struct HciEventContext *ctx) {
    ctx->scannedDeviceCount = 0;
}

static int findScannedDevice(struct HciEventContext *ctx, struct BdAddrT bdAddr) {
    for (int i = 0; i < ctx->scannedDeviceCount; i++) {
        if (memcmp(ctx->scannedDevices[i].bdAddr.addr, bdAddr.addr, kBdAddrLen) == 0) {
            return i;
        }
    }
    return -1;
}

static int addScannedDevice(struct HciEventContext *ctx, struct HciScannedDevice scanned) {
    if (ctx->scannedDeviceCount >= kHciScannedDeviceListSize) {
        return -1;
    }
    ctx->scannedDevices[ctx->scannedDeviceCount++] = scanned;
    return ctx->scannedDeviceCount;
}

static bool isSameBdAddr(const struct BdAddrT &a, const struct BdAddrT &b) {
    return memcmp(a.addr, b.addr, kBdAddrLen) == 0;
}

static void hciSend(struct HciEventContext *ctx, uint16_t len) {
    if (ctx->sendPacket != nullptr) {
        ctx->sendPacket(gHciTxBuffer, len, ctx->userData);
    }
}

static void logCommandCompleteError(uint16_t cmdOpcode, uint8_t status) {
    wiimoteLogError("HCI: %s (0x%04x) failed, status=0x%02x (%s)\n", hciOpcodeToString(cmdOpcode),
                    cmdOpcode, status, hciStatusCodeToString(status));
}

static uint32_t nowMs(struct HciEventContext *ctx) {
    if (ctx->getTimeMs != nullptr) {
        return ctx->getTimeMs(ctx->userData);
    }
    return millis();
}

static bool isFastReconnectCacheValid(struct HciEventContext *ctx) {
    if (!ctx->hasLastWiimote) {
        return false;
    }

    if (ctx->fastReconnectTtlMs == 0) {
        return false;
    }

    const uint32_t kElapsedMs = nowMs(ctx) - ctx->lastWiimoteSeenMs;
    return kElapsedMs <= ctx->fastReconnectTtlMs;
}

static void startInquiry(struct HciEventContext *ctx) {
    wiimoteLogInfo("HCI: Device initialized, starting inquiry\n");
    clearScannedDevices(ctx);
    HciInquiryParams inquiryParams = {0x9E8B33, 0x05, 0x00};
    const uint16_t kTxLen = makeCmdInquiry(gHciTxBuffer, inquiryParams);
    hciSend(ctx, kTxLen);
}

static void startCreateConnection(struct HciEventContext *ctx,
                                  const struct HciScannedDevice &target,
                                  bool isFastReconnect) {
    HciCreateConnectionParams createConnectionParams = {
        target.bdAddr, 0x0008, target.psrm, target.clkofs, 0x00,
    };
    const uint16_t kTxLen = makeCmdCreateConnection(gHciTxBuffer, createConnectionParams);
    hciSend(ctx, kTxLen);

    ctx->currentConnectTarget = target;
    ctx->hasCurrentConnectTarget = true;
    ctx->pendingFastReconnect = isFastReconnect;
}

static void fallbackFromFastReconnectToInquiry(struct HciEventContext *ctx) {
    if (!ctx->pendingFastReconnect) {
        return;
    }

    wiimoteLogWarn("HCI: Fast reconnect failed, falling back to inquiry\n");
    ctx->pendingFastReconnect = false;
    startInquiry(ctx);
}

void hciEventsInit(struct HciEventContext *ctx, HciSendPacketFunc sendPacket, void *userData) {
    if (ctx == nullptr) {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->sendPacket = sendPacket;
    ctx->userData = userData;
    ctx->fastReconnectTtlMs = kFastReconnectTtlMs;
    ctx->scanningEnabled = false;
    ctx->autoReconnectEnabled = true;
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

void hciEventsSetTimeProvider(struct HciEventContext *ctx, HciGetTimeMsFunc getTimeMs) {
    if (ctx == nullptr) {
        return;
    }

    ctx->getTimeMs = getTimeMs;
}

void hciEventsSetFastReconnectTtlMs(struct HciEventContext *ctx, uint32_t ttlMs) {
    if (ctx == nullptr) {
        return;
    }

    ctx->fastReconnectTtlMs = ttlMs;
}

void hciEventsResetDevice(struct HciEventContext *ctx) {
    if (ctx == nullptr) {
        return;
    }

    clearScannedDevices(ctx);
    ctx->scanningEnabled = false;
    const uint16_t kTxLen = makeCmdReset(gHciTxBuffer);
    hciSend(ctx, kTxLen);
}

static void handleCommandComplete(struct HciEventContext *ctx, const uint8_t *data) {
    const uint16_t kCmdOpcode = readUInt16Le(data + 1);
    const uint8_t kStatus = data[3];

    switch (kCmdOpcode) {
        case kHciOpcodeReset: {
            if (kStatus == 0x00) {
                wiimoteLogDebug("HCI: Reset successful\n");
                const uint16_t kTxLen = makeCmdReadBdAddr(gHciTxBuffer);
                hciSend(ctx, kTxLen);
            } else {
                logCommandCompleteError(kCmdOpcode, kStatus);
            }
            break;
        }

        case kHciOpcodeReadBdAddr: {
            if (kStatus == 0x00) {
                static const uint8_t kName[] = "ESP32-BT-L2CAP";
                const uint16_t kTxLen = makeCmdWriteLocalName(gHciTxBuffer, kName, sizeof(kName));
                hciSend(ctx, kTxLen);
            } else {
                logCommandCompleteError(kCmdOpcode, kStatus);
            }
            break;
        }

        case kHciOpcodeWriteLocalName: {
            if (kStatus == 0x00) {
                static const uint8_t kClassOfDevice[3] = {0x04, 0x05, 0x00};
                const uint16_t kTxLen = makeCmdWriteClassOfDevice(gHciTxBuffer, kClassOfDevice);
                hciSend(ctx, kTxLen);
            } else {
                logCommandCompleteError(kCmdOpcode, kStatus);
            }
            break;
        }

        case kHciOpcodeWriteClassOfDevice: {
            if (kStatus == 0x00) {
                const uint16_t kTxLen = makeCmdWriteScanEnable(gHciTxBuffer, 3);
                ctx->scanningEnabled = true;
                hciSend(ctx, kTxLen);
            } else {
                logCommandCompleteError(kCmdOpcode, kStatus);
            }
            break;
        }

        case kHciOpcodeWriteScanEnable: {
            if (kStatus == 0x00) {
                if (ctx->scanningEnabled || !ctx->deviceInited) {
                    if (ctx->autoReconnectEnabled && isFastReconnectCacheValid(ctx)) {
                        wiimoteLogInfo("HCI: Fast reconnect: trying cached Wiimote address\n");
                        startCreateConnection(ctx, ctx->lastWiimote, true);
                    } else {
                        startInquiry(ctx);
                    }
                }
                ctx->deviceInited = true;
            } else {
                logCommandCompleteError(kCmdOpcode, kStatus);
            }
            break;
        }

        case kHciOpcodeInquiryCancel:
            break;

        default:
            if (kStatus != 0x00) {
                wiimoteLogWarn("HCI: %s (0x%04x) completed with error status=0x%02x (%s)\n",
                               hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                               hciStatusCodeToString(kStatus));
            }
            break;
    }
}

static void handleCommandStatus(struct HciEventContext *ctx, const uint8_t *data) {
    const uint8_t kStatus = data[0];
    const uint8_t kNumHciCommandPackets = data[1];
    const uint16_t kCmdOpcode = readUInt16Le(data + 2);

    if (kStatus != 0x00) {
        wiimoteLogWarn("HCI: Command status for %s (0x%04x): status=0x%02x (%s), numPackets=%u\n",
                       hciOpcodeToString(kCmdOpcode), kCmdOpcode, kStatus,
                       hciStatusCodeToString(kStatus), kNumHciCommandPackets);

        if (kCmdOpcode == kHciOpcodeCreateConnection) {
            fallbackFromFastReconnectToInquiry(ctx);
        }
    }
}

static void handleInquiryComplete(struct HciEventContext *ctx, const uint8_t *data) {
    (void)data;
    hciEventsResetDevice(ctx);
}

static void handleInquiryResult(struct HciEventContext *ctx, const uint8_t *data) {
    const uint8_t kNum = data[0];
    wiimoteLogDebug("HCI: Inquiry result: found %d device(s)\n", kNum);

    for (int i = 0; i < kNum; i++) {
        const int kPos = 1 + ((6 + 1 + 2 + 3 + 2) * i);

        struct BdAddrT bdAddr;
        streamToBdAddr(bdAddr.addr, data + kPos);

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
            wiimoteLogInfo("HCI: Wiimote detected! Requesting remote name...\n");
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
    streamToBdAddr(bdAddr.addr, data + 1);

    char *name = (char *)(data + 7);

    wiimoteLogDebug("HCI: Remote name: %s\n", name);

    const int kIdx = findScannedDevice(ctx, bdAddr);
    if (kIdx < 0) {
        return;
    }

    if (strcmp("Nintendo RVL-CNT-01", name) == 0) {
        wiimoteLogInfo("HCI: Nintendo Wiimote confirmed! Initiating connection...\n");
        const uint16_t kCancelLen = makeCmdInquiryCancel(gHciTxBuffer);
        hciSend(ctx, kCancelLen);

        const struct HciScannedDevice kScanned = ctx->scannedDevices[kIdx];
        startCreateConnection(ctx, kScanned, false);
    }
}

static void handleConnectionComplete(struct HciEventContext *ctx, uint8_t *data) {
    const uint8_t kStatus = data[0];
    if (kStatus != 0x00) {
        wiimoteLogWarn("HCI: Connection complete with status=0x%02x (%s)\n", kStatus,
                       hciStatusCodeToString(kStatus));
        fallbackFromFastReconnectToInquiry(ctx);
        return;
    }

    const uint16_t kConnectionHandle = readUInt16Le(data + 1);
    wiimoteLogInfo("HCI: Connection complete! Handle: 0x%04x\n", kConnectionHandle);

    if (ctx->hasCurrentConnectTarget) {
        if (ctx->hasLastWiimote &&
            !isSameBdAddr(ctx->lastWiimote.bdAddr, ctx->currentConnectTarget.bdAddr)) {
            wiimoteLogInfo(
                "HCI: Connected to a different Wiimote, replacing fast reconnect cache\n");
            ctx->hasLastWiimote = false;
            ctx->lastWiimoteSeenMs = 0;
        }

        ctx->lastWiimote = ctx->currentConnectTarget;
        ctx->lastWiimoteSeenMs = nowMs(ctx);
        ctx->hasLastWiimote = true;
    }

    ctx->pendingFastReconnect = false;

    if (ctx->onAclConnected != nullptr) {
        ctx->onAclConnected(kConnectionHandle, ctx->userData);
    }
}

static void handleDisconnectionComplete(struct HciEventContext *ctx, const uint8_t *data) {
    const uint16_t kConnectionHandle = readUInt16Le(data + 1);
    const uint8_t kReason = data[3];

    wiimoteLogInfo("HCI: Disconnection complete! Handle: 0x%04x, Reason: 0x%02x (%s)\n",
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
        case kHciInquiryCompEvt:
            handleInquiryComplete(ctx, packet.data);
            break;

        case kHciInquiryResultEvt:
            handleInquiryResult(ctx, packet.data);
            break;

        case kHciConnectionCompEvt:
            handleConnectionComplete(ctx, packet.data);
            break;

        case kHciDisconnectionCompEvt:
            handleDisconnectionComplete(ctx, packet.data);
            break;

        case kHciRmtNameRequestCompEvt:
            handleRemoteNameRequestComplete(ctx, packet.data);
            break;

        case kHciCommandCompleteEvt:
            handleCommandComplete(ctx, packet.data);
            break;

        case kHciCommandStatusEvt:
            handleCommandStatus(ctx, packet.data);
            break;

        default:
            wiimoteLogDebug("HCI: Unhandled event 0x%02x (%s)\n", packet.eventCode,
                            hciEventCodeToString(packet.eventCode));
            break;
    }
}
