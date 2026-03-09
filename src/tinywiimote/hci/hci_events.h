// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __TINYWIIMOTE_HCI_EVENTS_H__
#define __TINYWIIMOTE_HCI_EVENTS_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../utils/hci_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*HciSendPacketFunc)(uint8_t* data, size_t len, void* userData);
typedef void (*HciAclConnectedFunc)(uint16_t connectionHandle, void* userData);
typedef void (*HciDisconnectedFunc)(uint16_t connectionHandle, uint8_t reason, void* userData);

typedef struct {
  struct bd_addr_t bdAddr;
  uint8_t psrm;
  uint16_t clkofs;
} hci_scanned_device_t;

#define HCI_SCANNED_DEVICE_LIST_SIZE 16

typedef struct {
  HciSendPacketFunc sendPacket;
  HciAclConnectedFunc onAclConnected;
  HciDisconnectedFunc onDisconnected;
  void* userData;

  hci_scanned_device_t scannedDevices[HCI_SCANNED_DEVICE_LIST_SIZE];
  int scannedDeviceCount;

  bool deviceInited;
} HciEventContext;

void hci_events_init(HciEventContext* ctx, HciSendPacketFunc sendPacket, void* userData);
void hci_events_set_callbacks(HciEventContext* ctx, HciAclConnectedFunc onAclConnected,
                              HciDisconnectedFunc onDisconnected);

void hci_events_reset_device(HciEventContext* ctx);
void hci_events_handle_event(HciEventContext* ctx, uint8_t eventCode, uint8_t len, uint8_t* data);

#ifdef __cplusplus
}
#endif

#endif // __TINYWIIMOTE_HCI_EVENTS_H__
