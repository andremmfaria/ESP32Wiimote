#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

#include "../../src/TinyWiimote.h"
#include "../../src/esp32wiimote/data_parser.h"
#include "../../src/tinywiimote/l2cap/l2cap_connection.h"
#include "../../src/tinywiimote/l2cap/l2cap_packets.h"
#include "../../src/tinywiimote/protocol/wiimote_protocol.h"

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
