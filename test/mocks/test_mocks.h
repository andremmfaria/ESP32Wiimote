#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

#include "../../src/TinyWiimote.h"
#include "../../src/esp32wiimote/data_parser.h"
#include "../../src/tinywiimote/l2cap/l2cap_connection.h"
#include "../../src/tinywiimote/l2cap/l2cap_packets.h"
#include "../../src/tinywiimote/protocol/wiimote_protocol.h"
#include "Arduino.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#include <algorithm>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unity.h>

// Mock state for TinyWiimote input simulation
extern bool mockHasData;
extern TinyWiimoteData mockData;

// Mock state for L2CAP packet capture
extern uint8_t mockLastPacket[256];
extern int mockLastPacketLen;
extern int mockSendCallCount;
extern uint16_t mockLastChannelHandle;
extern uint16_t mockLastRemoteCID;

// Additional TinyWiimote boundary mock state
extern bool mockDeviceIsInited;
extern int mockResetDeviceCallCount;
extern int mockHandleHciDataCallCount;
extern int mockTinyWiimoteInitCallCount;
extern struct TwHciInterface mockLastHciInterface;
extern bool mockTinyWiimoteConnected;
extern uint8_t mockBatteryLevel;
extern int mockRequestBatteryUpdateCallCount;
extern int mockReqAccelerometerCallCount;
extern bool mockLastReqAccelerometerUse;
extern uint32_t mockLastFastReconnectTtlMs;
extern bool mockSetLedsResult;
extern int mockSetLedsCallCount;
extern uint8_t mockLastLedsMask;
extern bool mockSetReportingModeResult;
extern int mockSetReportingModeCallCount;
extern uint8_t mockLastReportingMode;
extern bool mockLastReportingContinuous;
extern bool mockRequestStatusResult;
extern int mockRequestStatusCallCount;
extern bool mockWriteMemoryResult;
extern int mockWriteMemoryCallCount;
extern uint8_t mockLastWriteMemoryAddressSpace;
extern uint32_t mockLastWriteMemoryOffset;
extern const uint8_t *mockLastWriteMemoryData;
extern uint8_t mockLastWriteMemoryLen;
extern bool mockReadMemoryResult;
extern int mockReadMemoryCallCount;
extern uint8_t mockLastReadMemoryAddressSpace;
extern uint32_t mockLastReadMemoryOffset;
extern uint16_t mockLastReadMemorySize;
extern int mockSetScanEnabledCallCount;
extern bool mockLastScanEnabled;
extern bool mockStartDiscoveryResult;
extern int mockStartDiscoveryCallCount;
extern bool mockStopDiscoveryResult;
extern int mockStopDiscoveryCallCount;
extern bool mockDisconnectResult;
extern int mockDisconnectCallCount;
extern uint8_t mockLastDisconnectReason;
extern int mockSetAutoReconnectEnabledCallCount;
extern bool mockLastAutoReconnectEnabled;
extern int mockClearReconnectCacheCallCount;
extern BluetoothControllerState mockControllerState;

// ESP32 BT VHCI mock state (used by hci_queue.cpp and test_esp32_components)
extern bool gMockVhciSendAvailable;
extern uint8_t gMockVhciSentData[512];
extern size_t gMockVhciSentLen;
extern int gMockVhciSendCount;
extern esp_err_t mockVhciRegisterResult;
extern int mockVhciRegisterCallCount;
extern esp_vhci_host_callback_t *mockLastVhciCallback;

// ESP32 controller / Arduino runtime mock state
extern bool mockBtStartResult;
extern bool mockBtStarted;
extern uint8_t mockBtControllerStatus;

// FreeRTOS queue creation control
extern int mockQueueCreateCallCount;
extern int mockQueueCreateFailOnCall;

/**
 * Mock callback for L2CAP packet transmission
 * Validates ACL/L2CAP framing and captures payload for test assertions
 */
static inline void mockL2capRawSendCallback(uint8_t *data, size_t len) {
    mockSendCallCount++;

    if (data == nullptr || len == 0) {
        mockLastPacketLen = 0;
        return;
    }

    // Packet framing constants from hci_utils.h
    const size_t kHciH4AclPreambleSize = 5;
    const size_t kL2CapHeaderLen = 4;
    const uint8_t kH4TypeAcl = 2;

    // Validate minimum packet size (H4 + ACL header + L2CAP header)
    const size_t kMinPacketSize = kHciH4AclPreambleSize + kL2CapHeaderLen;
    TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(kMinPacketSize, len,
                                         "Packet too small - missing ACL/L2CAP headers");

    // Validate H4 packet type (byte 0)
    uint8_t h4Type = data[0];
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(kH4TypeAcl, h4Type,
                                    "Invalid H4 packet type - expected ACL (0x02)");

    // Extract and validate ACL header (bytes 1-4)
    uint16_t aclHandle = data[1] | ((data[2] & 0x0F) << 8);
    uint16_t aclLength = data[3] | (data[4] << 8);

    mockLastChannelHandle = aclHandle;

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(len - kHciH4AclPreambleSize, aclLength,
                                     "ACL length field doesn't match actual payload size");

    // Extract and validate L2CAP header (bytes 5-8)
    uint16_t l2capLength = data[5] | (data[6] << 8);
    uint16_t l2capCID = data[7] | (data[8] << 8);

    TEST_ASSERT_EQUAL_UINT16_MESSAGE(len - kMinPacketSize, l2capLength,
                                     "L2CAP length field doesn't match actual payload size");

    mockLastRemoteCID = l2capCID;

    // Store validated payload (strip both headers for test assertions)
    size_t payloadLen = len - kMinPacketSize;
    payloadLen = std::min<unsigned long>(payloadLen, sizeof(mockLastPacket));

    if (payloadLen > 0) {
        memcpy(mockLastPacket, data + kMinPacketSize, payloadLen);
    }
    mockLastPacketLen = (int)payloadLen;
}

/**
 * TinyWiimote input boundary mocks
 * Allow tests to inject Wiimote HID reports
 * Implementations in test/mocks/test_mocks.cpp
 */
// Forward declare (already declared in TinyWiimote.h, implemented here for native tests)

#endif  // TEST_MOCKS_H
