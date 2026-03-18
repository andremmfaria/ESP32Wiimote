// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md
//
// The short of it is...
//   You are free to:
//     Share — copy and redistribute the material in any medium or format
//     Adapt — remix, transform, and build upon the material
//   Under the following terms:
//     NonCommercial — You may not use the material for commercial purposes.
#include "TinyWiimote.h"

#include "esp32wiimote/bt_controller.h"
#include "tinywiimote/hci/hci_commands.h"
#include "tinywiimote/hci/hci_events.h"
#include "tinywiimote/l2cap/l2cap_connection.h"
#include "tinywiimote/l2cap/l2cap_packets.h"
#include "tinywiimote/l2cap/l2cap_signaling.h"
#include "tinywiimote/protocol/wiimote_extensions.h"
#include "tinywiimote/protocol/wiimote_protocol.h"
#include "tinywiimote/protocol/wiimote_reports.h"
#include "tinywiimote/protocol/wiimote_state.h"
#include "tinywiimote/utils/hci_utils.h"
#include "utils/hci_codes.h"
#include "utils/protocol_codes.h"
#include "utils/serial_logging.h"

#include <HardwareSerial.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

struct TinyWiimoteRuntime {
    TwHciInterface hciInterface;
    HciEventContext hciEventContext;
    L2capConnectionTable l2capConnections;
    L2capPacketSender packetSender;
    L2capSignaling l2capSignaling;
    WiimoteProtocol wiimoteProtocol;
    WiimoteState wiimoteState;
    WiimoteReports wiimoteReports;
    WiimoteExtensions wiimoteExtensions;
};

static TinyWiimoteRuntime gRuntime;

struct L2capFrameHeader {
    uint16_t connectionHandle;
    uint16_t channelId;
};

static void resetDeviceInternal();

static void sendHciPacketRaw(uint8_t *data, size_t len) {
    if (gRuntime.hciInterface.hciSendPacket != nullptr) {
        gRuntime.hciInterface.hciSendPacket(data, len);
    }
}

static void hciSendPacketAdapter(uint8_t *data, size_t len, void *userData) {
    (void)userData;
    sendHciPacketRaw(data, len);
}

static void onAclConnected(uint16_t connectionHandle, void *userData) {
    (void)userData;
    LOG_INFO("TinyWiimote: ACL connection established! Handle: 0x%04x\n", connectionHandle);
    LOG_DEBUG("TinyWiimote: Sending L2CAP connection request...\n");
    gRuntime.l2capSignaling.sendConnectionRequest(connectionHandle,
                                                  (uint16_t)L2capPsm::HidInterrupt, 0x0045);
}

static void onDisconnected(uint16_t connectionHandle, uint8_t reason, void *userData) {
    (void)connectionHandle;
    (void)reason;
    (void)userData;

    LOG_INFO("TinyWiimote: Wiimote disconnected! Handle: 0x%04x, Reason: 0x%02x (%s)\n",
             connectionHandle, reason, hciDisconnectionReasonToString(reason));
    LOG_INFO("Wiimote lost\n");
    gRuntime.wiimoteState.reset();
    resetDeviceInternal();
}

static void handleL2capData(const L2capFrameHeader &header, uint8_t *data, uint16_t len) {
    const uint16_t kCh = header.connectionHandle;

    if (len == 0 || data == nullptr) {
        return;
    }

    uint8_t code = data[0];

    switch (data[0]) {
        case (uint8_t)L2capSignalingCode::ConnectionResponse:
            if (len < 12) {
                return;
            }
            gRuntime.l2capSignaling.handleConnectionResponse(kCh, data, len);
            break;

        case (uint8_t)L2capSignalingCode::ConfigurationRequest:
            if (len < 12) {
                return;
            }
            gRuntime.l2capSignaling.handleConfigurationRequest(kCh, data, len);
            break;

        case (uint8_t)L2capSignalingCode::ConfigurationResponse:
            L2capSignaling::handleConfigurationResponse(data, len);
            break;

        case (uint8_t)WiimoteHidPrefix::InputReport: {
            if (len > 1) {
                LOG_DEBUG("TinyWiimote: HID input report=0x%02x (%s)\n", data[1],
                          wiimoteInputReportToString(data[1]));
            }
            if (!gRuntime.wiimoteState.isConnected()) {
                LOG_INFO("TinyWiimote: Wiimote detected! Setting LED and marking as connected\n");
                WiimoteLedCommand ledCommand = {0b0001};
                gRuntime.wiimoteProtocol.setLeds(kCh, ledCommand);
                gRuntime.wiimoteProtocol.requestStatus(kCh);
                LOG_INFO("Wiimote detected\n");
                gRuntime.wiimoteState.setConnected(true);
                if (gRuntime.wiimoteState.getUseAccelerometer()) {
                    WiimoteReportingModeCommand reportingModeCommand = {0x31, false};
                    gRuntime.wiimoteProtocol.setReportingMode(kCh, reportingModeCommand);
                }
            }
            gRuntime.wiimoteExtensions.handleReport(kCh, data, len);
            uint8_t idx = 0;  // Only one wiimote supported currently.
            gRuntime.wiimoteReports.put(idx, data, (uint8_t)len);
            break;
        }

        default:
            LOG_DEBUG("L2CAP: Unhandled code=0x%02x (signal=%s, hid=%s), len=%d data=%s\n", code,
                      l2capSignalCodeToString(code), wiimoteHidPrefixToString(code), len,
                      format2Hex(data, len));
            break;
    }
}

static void handleAclData(uint8_t *data, size_t len) {
    if (len < 8) {
        return;
    }

    uint16_t ch = ((data[1] & 0x0F) << 8) | data[0];
    uint8_t pbf = (data[1] & 0x30) >> 4;
    uint8_t bf = (data[1] & 0xC0) >> 6;

    if (pbf != 0b10 || bf != 0b00) {
        return;
    }

    uint16_t l2capLen = readUInt16Le(data + 4);
    L2capFrameHeader header;
    header.connectionHandle = ch;
    header.channelId = readUInt16Le(data + 6);
    if ((size_t)(8 + l2capLen) > len) {
        return;
    }

    handleL2capData(header, data + 8, l2capLen);
}

void handleHciData(uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) {
        return;
    }

    switch (data[0]) {
        case static_cast<uint8_t>(H4PacketType::Event):
            if (len >= 3 && (size_t)(3 + data[2]) <= len) {
                HciEventPacket eventPacket = {data[1], data[2], data + 3};
                hciEventsHandleEvent(&gRuntime.hciEventContext, eventPacket);
            }
            break;

        case static_cast<uint8_t>(H4PacketType::Acl):
            if (len >= 2) {
                handleAclData(data + 1, len - 1);
            }
            break;

        default:
            LOG_DEBUG("UNKNOWN EVENT len=%d data=%s\n", len, format2Hex(data, (uint16_t)len));
            break;
    }
}

static void resetDeviceInternal() {
    gRuntime.l2capConnections.clear();
    hciEventsResetDevice(&gRuntime.hciEventContext);
    gRuntime.wiimoteProtocol.init(&gRuntime.l2capConnections, &gRuntime.packetSender);
    gRuntime.wiimoteExtensions.init(&gRuntime.wiimoteState, &gRuntime.l2capConnections,
                                    &gRuntime.packetSender);
}

void tinyWiimoteResetDevice() {
    resetDeviceInternal();
    gRuntime.hciEventContext.deviceInited = true;
}

bool tinyWiimoteDeviceIsInited() {
    return gRuntime.hciEventContext.deviceInited;
}

bool tinyWiimoteIsConnected() {
    return gRuntime.wiimoteState.isConnected();
}

uint8_t tinyWiimoteGetBatteryLevel() {
    return gRuntime.wiimoteState.getBatteryLevel();
}

void tinyWiimoteRequestBatteryUpdate() {
    if (!gRuntime.wiimoteState.isConnected()) {
        LOG_WARN("TinyWiimote: Cannot request battery update - not connected\n");
        return;
    }

    uint16_t ch = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        LOG_ERROR("TinyWiimote: Cannot request battery update - no L2CAP connection\n");
        return;
    }

    LOG_DEBUG("TinyWiimote: Requesting battery status update\n");
    gRuntime.wiimoteProtocol.requestStatus(ch);
}

bool tinyWiimoteSetLeds(uint8_t ledMask) {
    if (!gRuntime.wiimoteState.isConnected()) {
        return false;
    }

    uint16_t ch = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        return false;
    }

    WiimoteLedCommand command = {ledMask};
    gRuntime.wiimoteProtocol.setLeds(ch, command);
    return true;
}

bool tinyWiimoteSetReportingMode(uint8_t mode, bool continuous) {
    if (!gRuntime.wiimoteState.isConnected()) {
        return false;
    }

    uint16_t ch = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        return false;
    }

    WiimoteReportingModeCommand command = {mode, continuous};
    gRuntime.wiimoteProtocol.setReportingMode(ch, command);
    return true;
}

bool tinyWiimoteRequestStatus() {
    if (!gRuntime.wiimoteState.isConnected()) {
        return false;
    }

    uint16_t ch = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        return false;
    }

    gRuntime.wiimoteProtocol.requestStatus(ch);
    return true;
}

bool tinyWiimoteWriteMemory(uint8_t addressSpace,
                            uint32_t offset,
                            const uint8_t *data,
                            uint8_t len) {
    if (!gRuntime.wiimoteState.isConnected()) {
        return false;
    }

    uint16_t ch = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        return false;
    }

    gRuntime.wiimoteProtocol.writeMemory(ch, static_cast<WiimoteAddressSpace>(addressSpace), offset,
                                         data, len);
    return true;
}

bool tinyWiimoteReadMemory(uint8_t addressSpace, uint32_t offset, uint16_t size) {
    if (!gRuntime.wiimoteState.isConnected()) {
        return false;
    }

    uint16_t ch = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        return false;
    }

    gRuntime.wiimoteProtocol.readMemory(ch, static_cast<WiimoteAddressSpace>(addressSpace), offset,
                                        size);
    return true;
}

int tinyWiimoteAvailable() {
    return gRuntime.wiimoteReports.available();
}

TinyWiimoteData tinyWiimoteRead() {
    return gRuntime.wiimoteReports.read();
}

void tinyWiimoteInit(struct TwHciInterface hciInterface) {
    LOG_DEBUG("TinyWiimote: Initializing TinyWiimote core...\n");

    gRuntime.hciInterface = hciInterface;

    LOG_DEBUG("TinyWiimote: Resetting wiimote state...\n");
    gRuntime.wiimoteState.reset();
    gRuntime.wiimoteReports.clear();

    LOG_DEBUG("TinyWiimote: Setting up packet sender...\n");
    gRuntime.packetSender.setSendCallback(sendHciPacketRaw);

    LOG_DEBUG("TinyWiimote: Initializing L2CAP connections...\n");
    gRuntime.l2capConnections.clear();
    gRuntime.l2capSignaling.init(&gRuntime.l2capConnections, &gRuntime.packetSender);

    LOG_DEBUG("TinyWiimote: Initializing Wiimote protocol...\n");
    gRuntime.wiimoteProtocol.init(&gRuntime.l2capConnections, &gRuntime.packetSender);

    LOG_DEBUG("TinyWiimote: Initializing Wiimote extensions...\n");
    gRuntime.wiimoteExtensions.init(&gRuntime.wiimoteState, &gRuntime.l2capConnections,
                                    &gRuntime.packetSender);

    LOG_DEBUG("TinyWiimote: Initializing HCI events...\n");
    hciEventsInit(&gRuntime.hciEventContext, hciSendPacketAdapter, nullptr);
    hciEventsSetCallbacks(&gRuntime.hciEventContext, onAclConnected, onDisconnected);

    LOG_INFO("TinyWiimote: Core initialization complete!\n");
}

void tinyWiimoteReqAccelerometer(bool use) {
    gRuntime.wiimoteState.setUseAccelerometer(use);
}

void tinyWiimoteSetFastReconnectTtlMs(uint32_t ttlMs) {
    hciEventsSetFastReconnectTtlMs(&gRuntime.hciEventContext, ttlMs);
}

void tinyWiimoteSetScanEnabled(bool enabled) {
    uint8_t tx[8] = {0};
    const uint8_t kMode = enabled ? 0x02 : 0x00;
    const uint16_t kTxLen = makeCmdWriteScanEnable(tx, kMode);
    sendHciPacketRaw(tx, kTxLen);
    gRuntime.hciEventContext.scanningEnabled = enabled;
}

bool tinyWiimoteStartDiscovery() {
    if (gRuntime.hciEventContext.scanningEnabled) {
        return false;
    }

    uint8_t tx[16] = {0};
    HciInquiryParams inquiryParams = {0x9E8B33, 0x08, 0xFF};
    const uint16_t kTxLen = makeCmdInquiry(tx, inquiryParams);
    sendHciPacketRaw(tx, kTxLen);
    gRuntime.hciEventContext.scanningEnabled = true;
    return true;
}

bool tinyWiimoteStopDiscovery() {
    if (!gRuntime.hciEventContext.scanningEnabled) {
        return false;
    }

    uint8_t tx[8] = {0};
    const uint16_t kTxLen = makeCmdInquiryCancel(tx);
    sendHciPacketRaw(tx, kTxLen);
    gRuntime.hciEventContext.scanningEnabled = false;
    return true;
}

bool tinyWiimoteDisconnect(uint8_t reason) {
    if (!gRuntime.wiimoteState.isConnected()) {
        return false;
    }

    uint16_t connectionHandle = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&connectionHandle) != 0) {
        return false;
    }

    uint8_t tx[8] = {0};
    const uint16_t kTxLen = makeCmdDisconnect(tx, connectionHandle, reason);
    sendHciPacketRaw(tx, kTxLen);
    return true;
}

void tinyWiimoteSetAutoReconnectEnabled(bool enabled) {
    gRuntime.hciEventContext.autoReconnectEnabled = enabled;
}

void tinyWiimoteClearReconnectCache() {
    gRuntime.hciEventContext.hasLastWiimote = false;
    memset(&gRuntime.hciEventContext.lastWiimote, 0, sizeof(gRuntime.hciEventContext.lastWiimote));
    gRuntime.hciEventContext.pendingFastReconnect = false;
}

BluetoothControllerState tinyWiimoteGetControllerState() {
    BluetoothControllerState state = {};
    state.initialized = gRuntime.hciEventContext.deviceInited;
    state.started = BluetoothController::isStarted();
    state.scanning = gRuntime.hciEventContext.scanningEnabled;
    state.connected = gRuntime.wiimoteState.isConnected();
    state.fastReconnectActive = gRuntime.hciEventContext.pendingFastReconnect;
    state.autoReconnectEnabled = gRuntime.hciEventContext.autoReconnectEnabled;

    uint16_t connectionHandle = 0;
    if (gRuntime.l2capConnections.getFirstConnectionHandle(&connectionHandle) == 0) {
        state.activeConnectionHandle = connectionHandle;
    }

    return state;
}
