// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_TINYWIIMOTE_HCI_EVENTS_H
#define ESP32_WIIMOTE_TINYWIIMOTE_HCI_EVENTS_H

#include "../utils/hci_utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*HciSendPacketFunc)(uint8_t *data, size_t len, void *userData);
typedef void (*HciAclConnectedFunc)(uint16_t connectionHandle, void *userData);
typedef void (*HciDisconnectedFunc)(uint16_t connectionHandle, uint8_t reason, void *userData);
typedef uint32_t (*HciGetTimeMsFunc)(void *userData);

static constexpr int kHciScannedDeviceListSize = 16;

struct HciScannedDevice {
    struct BdAddrT bdAddr;
    uint8_t psrm;
    uint16_t clkofs;
};

struct HciEventContext {
    HciSendPacketFunc sendPacket;
    HciAclConnectedFunc onAclConnected;
    HciDisconnectedFunc onDisconnected;
    HciGetTimeMsFunc getTimeMs;
    void *userData;

    struct HciScannedDevice scannedDevices[kHciScannedDeviceListSize];
    int scannedDeviceCount;

    struct HciScannedDevice lastWiimote;
    struct HciScannedDevice currentConnectTarget;
    uint32_t lastWiimoteSeenMs;
    uint32_t fastReconnectTtlMs;
    bool hasLastWiimote;
    bool hasCurrentConnectTarget;
    bool pendingFastReconnect;
    bool scanningEnabled;
    bool autoReconnectEnabled;

    bool deviceInited;
};

struct HciEventPacket {
    uint8_t eventCode;
    uint8_t len;
    uint8_t *data;
};

void hciEventsInit(struct HciEventContext *ctx, HciSendPacketFunc sendPacket, void *userData);
void hciEventsSetCallbacks(struct HciEventContext *ctx,
                           HciAclConnectedFunc onAclConnected,
                           HciDisconnectedFunc onDisconnected);
void hciEventsSetTimeProvider(struct HciEventContext *ctx, HciGetTimeMsFunc getTimeMs);
void hciEventsSetFastReconnectTtlMs(struct HciEventContext *ctx, uint32_t ttlMs);

void hciEventsResetDevice(struct HciEventContext *ctx);
void hciEventsHandleEvent(struct HciEventContext *ctx, const HciEventPacket &packet);

#endif  // ESP32_WIIMOTE_TINYWIIMOTE_HCI_EVENTS_H
