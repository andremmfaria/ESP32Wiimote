#include "l2cap_signaling.h"

L2capSignaling::L2capSignaling() : connections(nullptr), sender(nullptr) {}

void L2capSignaling::init(L2capConnectionTable* connectionTable, L2capPacketSender* packetSender) {
  connections = connectionTable;
  sender = packetSender;
}

void L2capSignaling::sendConnectionRequest(uint16_t ch, uint16_t psm, uint16_t cid) {
  if (sender == nullptr) {
    return;
  }

  uint8_t pos = 0;
  payload[pos++] = 0x02;  // CONNECTION REQUEST
  payload[pos++] = 0x01;  // Identifier
  payload[pos++] = 0x04;
  payload[pos++] = 0x00;
  payload[pos++] = (uint8_t)(psm & 0xFF);
  payload[pos++] = (uint8_t)(psm >> 8);
  payload[pos++] = (uint8_t)(cid & 0xFF);
  payload[pos++] = (uint8_t)(cid >> 8);

  sender->sendAclL2capPacket(ch, 0x0001, payload, pos);
}

void L2capSignaling::handleConnectionResponse(uint16_t ch, uint8_t* data, uint16_t len) {
  if (len < 12) {
    return;
  }

  if (connections == nullptr || sender == nullptr) {
    return;
  }

  uint16_t dstCID = ((uint16_t)data[5] << 8) | data[4];
  uint16_t result = ((uint16_t)data[9] << 8) | data[8];

  if (result != 0x0000) {
    return;
  }

  const L2capConnection connection(ch, dstCID);
  if (connections->addConnection(connection) < 0) {
    return;
  }

  uint8_t pos = 0;
  payload[pos++] = 0x04;  // CONFIGURATION REQUEST
  payload[pos++] = 0x02;  // Identifier
  payload[pos++] = 0x08;
  payload[pos++] = 0x00;
  payload[pos++] = (uint8_t)(dstCID & 0xFF);
  payload[pos++] = (uint8_t)(dstCID >> 8);
  payload[pos++] = 0x00;
  payload[pos++] = 0x00;
  payload[pos++] = 0x01;
  payload[pos++] = 0x02;
  payload[pos++] = 0x40;
  payload[pos++] = 0x00;

  sender->sendAclL2capPacket(ch, 0x0001, payload, pos);
}

void L2capSignaling::handleConfigurationRequest(uint16_t ch, uint8_t* data, uint16_t len) {
  if (len < 12) {
    return;
  }

  if (connections == nullptr || sender == nullptr) {
    return;
  }

  uint8_t identifier = data[1];
  uint16_t dataLen = ((uint16_t)data[3] << 8) | data[2];
  uint16_t flags = ((uint16_t)data[7] << 8) | data[6];

  if (flags != 0x0000 || dataLen != 0x08 || data[8] != 0x01 || data[9] != 0x02) {
    return;
  }

  uint16_t mtu = ((uint16_t)data[11] << 8) | data[10];
  uint16_t remoteCID = 0;
  if (connections->getRemoteCid(ch, &remoteCID) != 0) {
    return;
  }

  uint8_t pos = 0;
  payload[pos++] = 0x05;  // CONFIGURATION RESPONSE
  payload[pos++] = identifier;
  payload[pos++] = 0x0A;
  payload[pos++] = 0x00;
  payload[pos++] = (uint8_t)(remoteCID & 0xFF);
  payload[pos++] = (uint8_t)(remoteCID >> 8);
  payload[pos++] = 0x00;
  payload[pos++] = 0x00;
  payload[pos++] = 0x00;
  payload[pos++] = 0x00;
  payload[pos++] = 0x01;
  payload[pos++] = 0x02;
  payload[pos++] = (uint8_t)(mtu & 0xFF);
  payload[pos++] = (uint8_t)(mtu >> 8);

  sender->sendAclL2capPacket(ch, 0x0001, payload, pos);
}

void L2capSignaling::handleConfigurationResponse(uint8_t* data, uint16_t len) {
  (void)data;
  (void)len;
}
