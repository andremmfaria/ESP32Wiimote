// Test mock implementations for native tests
// Provides TinyWiimote boundary stubs and mock state variables

#include "test_mocks.h"
#include "../../src/tinywiimote/utils/hci_utils.h"

// Mock state variables with external linkage
bool mockHasData = false;
TinyWiimoteData mockData = {0, {0}, 0};

uint8_t mockLastPacket[256] = {0};
int mockLastPacketLen = 0;
int mockSendCallCount = 0;
uint16_t mockLastChannelHandle = 0;
uint16_t mockLastRemoteCID = 0;

// TinyWiimote input boundary mocks
int TinyWiimoteAvailable(void) {
  return mockHasData ? 1 : 0;
}

TinyWiimoteData TinyWiimoteRead(void) {
  mockHasData = false;
  return mockData;
}

// L2CAP packet framing functions (real implementations for testing)
uint16_t make_l2cap_packet(uint8_t* buf, uint16_t channelID, uint8_t* data, uint16_t len) {
  UINT16_TO_STREAM(buf, len);
  UINT16_TO_STREAM(buf, channelID);
  ARRAY_TO_STREAM(buf, data, len);
  return L2CAP_HEADER_LEN + len;
}

uint16_t make_acl_l2cap_packet(uint8_t* buf, uint16_t ch, uint8_t pbf, uint8_t bf,
                                uint16_t channelID, uint8_t* data, uint8_t len) {
  uint8_t* l2capBuf = buf + HCI_H4_ACL_PREAMBLE_SIZE;
  uint16_t l2capLen = make_l2cap_packet(l2capBuf, channelID, data, len);

  UINT8_TO_STREAM(buf, H4_TYPE_ACL);
  UINT8_TO_STREAM(buf, ch & 0xFF);
  UINT8_TO_STREAM(buf, ((ch >> 8) & 0x0F) | (pbf << 4) | (bf << 6));
  UINT16_TO_STREAM(buf, l2capLen);

  return HCI_H4_ACL_PREAMBLE_SIZE + l2capLen;
}

// L2capPacketSender implementations for native tests
// Captures connection info and uses real packet framing
L2capPacketSender::L2capPacketSender() : sendCallback(nullptr), tmpQueueData{0} {}

void L2capPacketSender::setSendCallback(L2capRawSendFunc callback) {
  sendCallback = callback;
}

void L2capPacketSender::sendAclL2capPacket(uint16_t ch, uint16_t remoteCID,
                                           uint8_t* payload, uint16_t payloadLen) {
  // Capture connection info for validation in callback
  mockLastChannelHandle = ch;
  mockLastRemoteCID = remoteCID;

  if (sendCallback == nullptr || payload == nullptr) {
    return;
  }

  const uint8_t pbf = 0b10;  // Packet boundary: first non-automatically-flushable packet
  const uint8_t bf = 0b00;   // Broadcast: point-to-point
  
  const uint16_t packetLen = make_acl_l2cap_packet(tmpQueueData, ch, pbf, bf, 
                                                    remoteCID, payload, 
                                                    (uint8_t)payloadLen);
  sendCallback(tmpQueueData, packetLen);
}
