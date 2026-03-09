#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unity.h>

#include "../../src/TinyWiimote.h"
#include "../../src/esp32wiimote/data_parser.h"
#include "../../src/tinywiimote/l2cap/l2cap_connection.h"
#include "../../src/tinywiimote/l2cap/l2cap_packets.h"
#include "../../src/tinywiimote/protocol/wiimote_protocol.h"

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
static inline void mockL2capRawSendCallback(uint8_t* data, size_t len) {
  mockSendCallCount++;

  if (data == nullptr || len == 0) {
    mockLastPacketLen = 0;
    return;
  }

  // Packet framing constants from hci_utils.h
  const size_t HCI_H4_ACL_PREAMBLE_SIZE = 5;
  const size_t L2CAP_HEADER_LEN = 4;
  const uint8_t H4_TYPE_ACL = 2;

  // Validate minimum packet size (H4 + ACL header + L2CAP header)
  const size_t minPacketSize = HCI_H4_ACL_PREAMBLE_SIZE + L2CAP_HEADER_LEN;
  TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(minPacketSize, len, 
    "Packet too small - missing ACL/L2CAP headers");

  // Validate H4 packet type (byte 0)
  uint8_t h4Type = data[0];
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(H4_TYPE_ACL, h4Type, 
    "Invalid H4 packet type - expected ACL (0x02)");

  // Extract and validate ACL header (bytes 1-4)
  uint16_t aclHandle = data[1] | ((data[2] & 0x0F) << 8);
  uint16_t aclLength = data[3] | (data[4] << 8);
  
  uint16_t expectedAclHandle = mockLastChannelHandle & 0x0FFF;
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(expectedAclHandle, aclHandle,
    "ACL connection handle mismatch");
  
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(len - HCI_H4_ACL_PREAMBLE_SIZE, aclLength,
    "ACL length field doesn't match actual payload size");

  // Extract and validate L2CAP header (bytes 5-8)
  uint16_t l2capLength = data[5] | (data[6] << 8);
  uint16_t l2capCID = data[7] | (data[8] << 8);
  
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(len - minPacketSize, l2capLength,
    "L2CAP length field doesn't match actual payload size");
  
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(mockLastRemoteCID, l2capCID,
    "L2CAP channel ID mismatch");

  // Store validated payload (strip both headers for test assertions)
  size_t payloadLen = len - minPacketSize;
  if (payloadLen > sizeof(mockLastPacket)) {
    payloadLen = sizeof(mockLastPacket);
  }
  
  if (payloadLen > 0) {
    memcpy(mockLastPacket, data + minPacketSize, payloadLen);
  }
  mockLastPacketLen = (int)payloadLen;
}

/**
 * TinyWiimote input boundary mocks
 * Allow tests to inject Wiimote HID reports
 * Implementations in test/mocks/test_mocks.cpp
 */
// Forward declare (already declared in TinyWiimote.h, implemented here for native tests)

#endif // TEST_MOCKS_H
