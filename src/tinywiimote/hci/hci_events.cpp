// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "hci_events.h"

#include <string.h>

#include "hci_types.h"
#include "hci_commands.h"
#include "../../utils/serial_logging.h"

static uint8_t g_hciTxBuffer[256];

static void clear_scanned_devices(struct HciEventContext* ctx) {
  ctx->scannedDeviceCount = 0;
}

static int find_scanned_device(struct HciEventContext* ctx, struct bd_addr_t bdAddr) {
  for (int i = 0; i < ctx->scannedDeviceCount; i++) {
    if (memcmp(ctx->scannedDevices[i].bdAddr.addr, bdAddr.addr, BD_ADDR_LEN) == 0) {
      return i;
    }
  }
  return -1;
}

static int add_scanned_device(struct HciEventContext* ctx, struct HciScannedDevice scanned) {
  if (ctx->scannedDeviceCount >= HCI_SCANNED_DEVICE_LIST_SIZE) {
    return -1;
  }
  ctx->scannedDevices[ctx->scannedDeviceCount++] = scanned;
  return ctx->scannedDeviceCount;
}

static void hci_send(struct HciEventContext* ctx, uint16_t len) {
  if (ctx->sendPacket != 0) {
    ctx->sendPacket(g_hciTxBuffer, len, ctx->userData);
  }
}

void hci_events_init(struct HciEventContext* ctx, HciSendPacketFunc sendPacket, void* userData) {
  if (ctx == 0) {
    return;
  }

  memset(ctx, 0, sizeof(*ctx));
  ctx->sendPacket = sendPacket;
  ctx->userData = userData;
}

void hci_events_set_callbacks(struct HciEventContext* ctx, HciAclConnectedFunc onAclConnected,
                              HciDisconnectedFunc onDisconnected) {
  if (ctx == 0) {
    return;
  }

  ctx->onAclConnected = onAclConnected;
  ctx->onDisconnected = onDisconnected;
}

void hci_events_reset_device(struct HciEventContext* ctx) {
  if (ctx == 0) {
    return;
  }

  clear_scanned_devices(ctx);
  const uint16_t txLen = make_cmd_reset(g_hciTxBuffer);
  hci_send(ctx, txLen);
}

static void handle_command_complete(struct HciEventContext* ctx, uint8_t* data) {
  const uint16_t cmdOpcode = READ_UINT16_LE(data + 1);

  switch (cmdOpcode) {
    case HCI_OPCODE_RESET: {
      if (data[3] == 0x00) {
        LOG_DEBUG("HCI: Reset successful\n");
        const uint16_t txLen = make_cmd_read_bd_addr(g_hciTxBuffer);
        hci_send(ctx, txLen);
      } else {
        LOG_ERROR("HCI: Reset failed with status=0x%02x\n", data[3]);
      }
      break;
    }

    case HCI_OPCODE_READ_BD_ADDR: {
      if (data[3] == 0x00) {
        static const uint8_t kName[] = "ESP32-BT-L2CAP";
        const uint16_t txLen = make_cmd_write_local_name(g_hciTxBuffer, kName, sizeof(kName));
        hci_send(ctx, txLen);
      }
      break;
    }

    case HCI_OPCODE_WRITE_LOCAL_NAME: {
      if (data[3] == 0x00) {
        static const uint8_t kClassOfDevice[3] = {0x04, 0x05, 0x00};
        const uint16_t txLen = make_cmd_write_class_of_device(g_hciTxBuffer, kClassOfDevice);
        hci_send(ctx, txLen);
      }
      break;
    }

    case HCI_OPCODE_WRITE_CLASS_OF_DEVICE: {
      if (data[3] == 0x00) {
        const uint16_t txLen = make_cmd_write_scan_enable(g_hciTxBuffer, 3);
        hci_send(ctx, txLen);
      }
      break;
    }

    case HCI_OPCODE_WRITE_SCAN_ENABLE: {
      if (data[3] == 0x00) {
        LOG_INFO("HCI: Device initialized, starting inquiry\n");
        clear_scanned_devices(ctx);
        const uint16_t txLen = make_cmd_inquiry(g_hciTxBuffer, 0x9E8B33, 0x05, 0x00);
        hci_send(ctx, txLen);
        ctx->deviceInited = true;
      }
      break;
    }

    case HCI_OPCODE_INQUIRY_CANCEL:
      break;

    default:
      break;
  }
}

static void handle_command_status(struct HciEventContext* ctx, uint8_t* data) {
  (void)ctx;
  (void)data;
}

static void handle_inquiry_complete(struct HciEventContext* ctx, uint8_t* data) {
  (void)data;
  hci_events_reset_device(ctx);
}

static void handle_inquiry_result(struct HciEventContext* ctx, uint8_t* data) {
  const uint8_t num = data[0];
  LOG_DEBUG("HCI: Inquiry result: found %d device(s)\n", num);

  for (int i = 0; i < num; i++) {
    const int pos = 1 + (6 + 1 + 2 + 3 + 2) * i;

    struct bd_addr_t bdAddr;
    STREAM_TO_BDADDR(bdAddr.addr, data + pos);

    int idx = find_scanned_device(ctx, bdAddr);
    if (idx != -1) {
      continue;
    }

    struct HciScannedDevice scanned;
    scanned.bdAddr = bdAddr;
    scanned.psrm = data[pos + 6];
    scanned.clkofs = ((0x80 | data[pos + 12]) << 8) | (data[pos + 13]);

    idx = add_scanned_device(ctx, scanned);
    if (idx < 0) {
      continue;
    }

    // Filter for Wiimote class-of-device: [04 25 00]
    if (data[pos + 9] == 0x04 && data[pos + 10] == 0x25 && data[pos + 11] == 0x00) {
      LOG_INFO("HCI: Wiimote detected! Requesting remote name...\n");
      const uint16_t txLen = make_cmd_remote_name_request(g_hciTxBuffer, scanned.bdAddr,
                                                           scanned.psrm, scanned.clkofs);
      hci_send(ctx, txLen);
    }
  }
}

static void handle_remote_name_request_complete(struct HciEventContext* ctx, uint8_t* data) {
  struct bd_addr_t bdAddr;
  STREAM_TO_BDADDR(bdAddr.addr, data + 1);

  char* name = (char*)(data + 7);

  LOG_DEBUG("HCI: Remote name: %s\n", name);

  const int idx = find_scanned_device(ctx, bdAddr);
  if (idx < 0) {
    return;
  }

  if (strcmp("Nintendo RVL-CNT-01", name) == 0) {
    LOG_INFO("HCI: Nintendo Wiimote confirmed! Initiating connection...\n");
    const uint16_t cancelLen = make_cmd_inquiry_cancel(g_hciTxBuffer);
    hci_send(ctx, cancelLen);

    const struct HciScannedDevice scanned = ctx->scannedDevices[idx];
    const uint16_t packetType = 0x0008;
    const uint8_t allowRoleSwitch = 0x00;

    const uint16_t txLen = make_cmd_create_connection(g_hciTxBuffer, scanned.bdAddr,
                                                       packetType, scanned.psrm,
                                                       scanned.clkofs, allowRoleSwitch);
    hci_send(ctx, txLen);
  }
}

static void handle_connection_complete(struct HciEventContext* ctx, uint8_t* data) {
  const uint16_t connectionHandle = READ_UINT16_LE(data + 1);
  LOG_INFO("HCI: Connection complete! Handle: 0x%04x\n", connectionHandle);
  if (ctx->onAclConnected != 0) {
    ctx->onAclConnected(connectionHandle, ctx->userData);
  }
}

static void handle_disconnection_complete(struct HciEventContext* ctx, uint8_t* data) {
  const uint16_t connectionHandle = READ_UINT16_LE(data + 1);
  const uint8_t reason = data[3];

  LOG_INFO("HCI: Disconnection complete! Handle: 0x%04x, Reason: 0x%02x\n", connectionHandle, reason);

  if (ctx->onDisconnected != 0) {
    ctx->onDisconnected(connectionHandle, reason, ctx->userData);
  }
}

void hci_events_handle_event(struct HciEventContext* ctx, uint8_t eventCode, uint8_t len, uint8_t* data) {
  (void)len;

  if (ctx == 0 || data == 0) {
    return;
  }

  switch (eventCode) {
    case HCI_INQUIRY_COMP_EVT:
      handle_inquiry_complete(ctx, data);
      break;

    case HCI_INQUIRY_RESULT_EVT:
      handle_inquiry_result(ctx, data);
      break;

    case HCI_CONNECTION_COMP_EVT:
      handle_connection_complete(ctx, data);
      break;

    case HCI_DISCONNECTION_COMP_EVT:
      handle_disconnection_complete(ctx, data);
      break;

    case HCI_RMT_NAME_REQUEST_COMP_EVT:
      handle_remote_name_request_complete(ctx, data);
      break;

    case HCI_COMMAND_COMPLETE_EVT:
      handle_command_complete(ctx, data);
      break;

    case HCI_COMMAND_STATUS_EVT:
      handle_command_status(ctx, data);
      break;

    default:
      break;
  }
}
