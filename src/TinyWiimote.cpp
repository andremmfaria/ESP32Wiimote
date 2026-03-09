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

#include <HardwareSerial.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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
#include "utils/serial_logging.h"

#define L2CAP_CONNECT_RES 0x03
#define L2CAP_CONFIG_REQ 0x04
#define L2CAP_CONFIG_RES 0x05
#define BTCODE_HID 0xA1

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

static void resetDeviceInternal(void);

static void send_hci_packet_raw(uint8_t* data, size_t len) {
  if (g_runtime.hciInterface.hci_send_packet != 0) {
    g_runtime.hciInterface.hci_send_packet(data, len);
  }
}

static void hci_send_packet_adapter(uint8_t* data, size_t len, void* userData) {
  (void)userData;
  send_hci_packet_raw(data, len);
}

static void on_acl_connected(uint16_t connectionHandle, void* userData) {
  (void)userData;
  LOG_INFO("TinyWiimote: ACL connection established! Handle: 0x%04x\n", connectionHandle);
  LOG_DEBUG("TinyWiimote: Sending L2CAP connection request...\n");
  g_runtime.l2capSignaling.sendConnectionRequest(connectionHandle, 0x0013, 0x0045);
}

static void on_disconnected(uint16_t connectionHandle, uint8_t reason, void* userData) {
  (void)connectionHandle;
  (void)reason;
  (void)userData;

  LOG_INFO("TinyWiimote: Wiimote disconnected! Handle: 0x%04x, Reason: 0x%02x\n", connectionHandle, reason);
  LOG_INFO("Wiimote lost\n");
  g_runtime.wiimoteState.reset();
  resetDeviceInternal();
}

static void handleL2capData(uint16_t ch, uint16_t channelID, uint8_t* data, uint16_t len) {
  (void)channelID;

  if (len == 0 || data == 0) {
    return;
  }
  
  uint8_t code = data[0];

  switch (data[0]) {
    case L2CAP_CONNECT_RES:
      if (len < 12) {
        return;
      }
      g_runtime.l2capSignaling.handleConnectionResponse(ch, data, len);
      break;

    case L2CAP_CONFIG_REQ:
      if (len < 12) {
        return;
      }
      g_runtime.l2capSignaling.handleConfigurationRequest(ch, data, len);
      break;

    case L2CAP_CONFIG_RES:
      g_runtime.l2capSignaling.handleConfigurationResponse(data, len);
      break;

    case BTCODE_HID: {
      if (!g_runtime.wiimoteState.isConnected()) {
        LOG_INFO("TinyWiimote: Wiimote detected! Setting LED and marking as connected\n");
        g_runtime.wiimoteProtocol.setLeds(ch, 0b0001);
        LOG_INFO("Wiimote detected\n");
        g_runtime.wiimoteState.setConnected(true);
        if (g_runtime.wiimoteState.getUseAccelerometer()) {
          g_runtime.wiimoteProtocol.setReportingMode(ch, 0x31, false);
        }
      }
      g_runtime.wiimoteExtensions.handleReport(ch, data, len);
      uint8_t idx = 0;  // Only one wiimote supported currently.
      g_runtime.wiimoteReports.put(idx, data, (uint8_t)len);
      break;
    }

    default:
      LOG_DEBUG("L2CAP len=%d data=%s\n", len, format2Hex(data, len));
      break;
  }
}

static void handleAclData(uint8_t* data, size_t len) {
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
  uint16_t channelID = READ_UINT16_LE(data + 6);
  if ((size_t)(8 + l2capLen) > len) {
    return;
  }

  handleL2capData(ch, channelID, data + 8, l2capLen);
}

void handleHciData(uint8_t* data, size_t len) {
  if (data == 0 || len == 0) {
    return;
  }

  switch (data[0]) {
    case H4_TYPE_EVENT:
      if (len >= 3 && (size_t)(3 + data[2]) <= len) {
        hci_events_handle_event(&g_runtime.hciEventContext, data[1], data[2], data + 3);
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

static void resetDeviceInternal(void) {
  g_runtime.l2capConnections.clear();
  hci_events_reset_device(&g_runtime.hciEventContext);
  g_runtime.wiimoteProtocol.init(&g_runtime.l2capConnections, &g_runtime.packetSender);
  g_runtime.wiimoteExtensions.init(&g_runtime.wiimoteState,
                                   &g_runtime.l2capConnections,
                                   &g_runtime.packetSender);
}

void TinyWiimoteResetDevice(void) {
  resetDeviceInternal();
  g_runtime.hciEventContext.deviceInited = true;
}

bool TinyWiimoteDeviceIsInited(void) {
  return g_runtime.hciEventContext.deviceInited;
}

bool TinyWiimoteIsConnected(void) {
  return g_runtime.wiimoteState.isConnected();
}

uint8_t TinyWiimoteGetBatteryLevel(void) {
  return g_runtime.wiimoteState.getBatteryLevel();
}

int TinyWiimoteAvailable(void) {
  return g_runtime.wiimoteReports.available();
}

TinyWiimoteData TinyWiimoteRead(void) {
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
  g_runtime.wiimoteExtensions.init(&g_runtime.wiimoteState,
                                   &g_runtime.l2capConnections,
                                   &g_runtime.packetSender);

  LOG_DEBUG("TinyWiimote: Initializing HCI events...\n");
  hci_events_init(&g_runtime.hciEventContext, hci_send_packet_adapter, 0);
  hci_events_set_callbacks(&g_runtime.hciEventContext, on_acl_connected, on_disconnected);
  
  LOG_INFO("TinyWiimote: Core initialization complete!\n");
}

void TinyWiimoteReqAccelerometer(bool use) {
  g_runtime.wiimoteState.setUseAccelerometer(use);
}
