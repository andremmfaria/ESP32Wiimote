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

#define CONFIG_CLASSIC_BT_ENABLED

#include "TinyWiimote.h"

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

static TinyWiimoteRuntime g_runtime;

struct L2capFrameHeader {
    uint16_t connectionHandle;
    uint16_t channelId;
};

static void resetDeviceInternal();

static void send_hci_packet_raw(uint8_t *data, size_t len) {
    if (g_runtime.hciInterface.hci_send_packet != nullptr) {
        g_runtime.hciInterface.hci_send_packet(data, len);
    }
}

static void hci_send_packet_adapter(uint8_t *data, size_t len, void *userData) {
    (void)userData;
    send_hci_packet_raw(data, len);
}

static void on_acl_connected(uint16_t connectionHandle, void *userData) {
    (void)userData;
    LOG_INFO("TinyWiimote: ACL connection established! Handle: 0x%04x\n", connectionHandle);
    LOG_DEBUG("TinyWiimote: Sending L2CAP connection request...\n");
    g_runtime.l2capSignaling.sendConnectionRequest(connectionHandle,
                                                   (uint16_t)L2capPsm::HID_INTERRUPT, 0x0045);
}

static void on_disconnected(uint16_t connectionHandle, uint8_t reason, void *userData) {
    (void)connectionHandle;
    (void)reason;
    (void)userData;

    LOG_INFO("TinyWiimote: Wiimote disconnected! Handle: 0x%04x, Reason: 0x%02x (%s)\n",
             connectionHandle, reason, hciDisconnectionReasonToString(reason));
    LOG_INFO("Wiimote lost\n");
    g_runtime.wiimoteState.reset();
    resetDeviceInternal();
}

static void handleL2capData(const L2capFrameHeader &header, uint8_t *data, uint16_t len) {
    const uint16_t ch = header.connectionHandle;

    if (len == 0 || data == nullptr) {
        return;
    }

    uint8_t code = data[0];

    switch (data[0]) {
        case (uint8_t)L2capSignalingCode::CONNECTION_RESPONSE:
            if (len < 12) {
                return;
            }
            g_runtime.l2capSignaling.handleConnectionResponse(ch, data, len);
            break;

        case (uint8_t)L2capSignalingCode::CONFIGURATION_REQUEST:
            if (len < 12) {
                return;
            }
            g_runtime.l2capSignaling.handleConfigurationRequest(ch, data, len);
            break;

        case (uint8_t)L2capSignalingCode::CONFIGURATION_RESPONSE:
            L2capSignaling::handleConfigurationResponse(data, len);
            break;

        case (uint8_t)WiimoteHidPrefix::INPUT_REPORT: {
            if (len > 1) {
                LOG_DEBUG("TinyWiimote: HID input report=0x%02x (%s)\n", data[1],
                          wiimoteInputReportToString(data[1]));
            }
            if (!g_runtime.wiimoteState.isConnected()) {
                LOG_INFO("TinyWiimote: Wiimote detected! Setting LED and marking as connected\n");
                WiimoteLedCommand ledCommand = {0b0001};
                g_runtime.wiimoteProtocol.setLeds(ch, ledCommand);
                g_runtime.wiimoteProtocol.requestStatus(ch);
                LOG_INFO("Wiimote detected\n");
                g_runtime.wiimoteState.setConnected(true);
                if (g_runtime.wiimoteState.getUseAccelerometer()) {
                    WiimoteReportingModeCommand reportingModeCommand = {0x31, false};
                    g_runtime.wiimoteProtocol.setReportingMode(ch, reportingModeCommand);
                }
            }
            g_runtime.wiimoteExtensions.handleReport(ch, data, len);
            uint8_t idx = 0;  // Only one wiimote supported currently.
            g_runtime.wiimoteReports.put(idx, data, (uint8_t)len);
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

    uint16_t l2capLen = READ_UINT16_LE(data + 4);
    L2capFrameHeader header;
    header.connectionHandle = ch;
    header.channelId = READ_UINT16_LE(data + 6);
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
        case H4_TYPE_EVENT:
            if (len >= 3 && (size_t)(3 + data[2]) <= len) {
                HciEventPacket eventPacket = {data[1], data[2], data + 3};
                hci_events_handle_event(&g_runtime.hciEventContext, eventPacket);
            }
            break;

        case H4_TYPE_ACL:
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
    g_runtime.l2capConnections.clear();
    hci_events_reset_device(&g_runtime.hciEventContext);
    g_runtime.wiimoteProtocol.init(&g_runtime.l2capConnections, &g_runtime.packetSender);
    g_runtime.wiimoteExtensions.init(&g_runtime.wiimoteState, &g_runtime.l2capConnections,
                                     &g_runtime.packetSender);
}

void TinyWiimoteResetDevice() {
    resetDeviceInternal();
    g_runtime.hciEventContext.deviceInited = true;
}

bool TinyWiimoteDeviceIsInited() {
    return g_runtime.hciEventContext.deviceInited;
}

bool TinyWiimoteIsConnected() {
    return g_runtime.wiimoteState.isConnected();
}

uint8_t TinyWiimoteGetBatteryLevel() {
    return g_runtime.wiimoteState.getBatteryLevel();
}

void TinyWiimoteRequestBatteryUpdate() {
    if (!g_runtime.wiimoteState.isConnected()) {
        LOG_WARN("TinyWiimote: Cannot request battery update - not connected\n");
        return;
    }

    uint16_t ch = 0;
    if (g_runtime.l2capConnections.getFirstConnectionHandle(&ch) != 0) {
        LOG_ERROR("TinyWiimote: Cannot request battery update - no L2CAP connection\n");
        return;
    }

    LOG_DEBUG("TinyWiimote: Requesting battery status update\n");
    g_runtime.wiimoteProtocol.requestStatus(ch);
}

int TinyWiimoteAvailable() {
    return g_runtime.wiimoteReports.available();
}

TinyWiimoteData TinyWiimoteRead() {
    return g_runtime.wiimoteReports.read();
}

void TinyWiimoteInit(struct TwHciInterface hciInterface) {
    LOG_DEBUG("TinyWiimote: Initializing TinyWiimote core...\n");

    g_runtime.hciInterface = hciInterface;

    LOG_DEBUG("TinyWiimote: Resetting wiimote state...\n");
    g_runtime.wiimoteState.reset();
    g_runtime.wiimoteReports.clear();

    LOG_DEBUG("TinyWiimote: Setting up packet sender...\n");
    g_runtime.packetSender.setSendCallback(send_hci_packet_raw);

    LOG_DEBUG("TinyWiimote: Initializing L2CAP connections...\n");
    g_runtime.l2capConnections.clear();
    g_runtime.l2capSignaling.init(&g_runtime.l2capConnections, &g_runtime.packetSender);

    LOG_DEBUG("TinyWiimote: Initializing Wiimote protocol...\n");
    g_runtime.wiimoteProtocol.init(&g_runtime.l2capConnections, &g_runtime.packetSender);

    LOG_DEBUG("TinyWiimote: Initializing Wiimote extensions...\n");
    g_runtime.wiimoteExtensions.init(&g_runtime.wiimoteState, &g_runtime.l2capConnections,
                                     &g_runtime.packetSender);

    LOG_DEBUG("TinyWiimote: Initializing HCI events...\n");
    hci_events_init(&g_runtime.hciEventContext, hci_send_packet_adapter, nullptr);
    hci_events_set_callbacks(&g_runtime.hciEventContext, on_acl_connected, on_disconnected);

    LOG_INFO("TinyWiimote: Core initialization complete!\n");
}

void TinyWiimoteReqAccelerometer(bool use) {
    g_runtime.wiimoteState.setUseAccelerometer(use);
}
