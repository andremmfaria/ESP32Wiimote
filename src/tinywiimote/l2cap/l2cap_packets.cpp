// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "l2cap_packets.h"
#include "../utils/hci_utils.h"

L2capPacketSender::L2capPacketSender() : sendCallback(nullptr) {}

void L2capPacketSender::setSendCallback(L2capRawSendFunc callback) {
  sendCallback = callback;
}

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

void L2capPacketSender::sendAclL2capPacket(uint16_t ch, uint16_t remoteCID, uint8_t* payload,
                                           uint16_t payloadLen) {
  if (sendCallback == nullptr || payloadLen > 255) {
    return;
  }

  const uint8_t pbf = 0b10;
  const uint8_t bf = 0b00;

  const uint16_t packetLen = make_acl_l2cap_packet(tmpQueueData, ch, pbf, bf, remoteCID,
                                                    payload, (uint8_t)payloadLen);
  sendCallback(tmpQueueData, packetLen);
}
