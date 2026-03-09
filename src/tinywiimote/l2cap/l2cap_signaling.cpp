#include "l2cap_signaling.h"
#include "l2cap_connection.h"
#include "l2cap_packets.h"

static uint8_t l2capPayload[64];

void l2cap_signaling_send_connection_request(uint16_t ch, uint16_t psm, uint16_t cid) {
  uint8_t pos = 0;
  l2capPayload[pos++] = 0x02;  // CONNECTION REQUEST
  l2capPayload[pos++] = 0x01;  // Identifier
  l2capPayload[pos++] = 0x04;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = (uint8_t)(psm & 0xFF);
  l2capPayload[pos++] = (uint8_t)(psm >> 8);
  l2capPayload[pos++] = (uint8_t)(cid & 0xFF);
  l2capPayload[pos++] = (uint8_t)(cid >> 8);

  send_acl_l2cap_packet(ch, 0x0001, l2capPayload, pos);
}

void l2cap_signaling_handle_connection_response(uint16_t ch, uint8_t* data, uint16_t len) {
  if (len < 12) {
    return;
  }

  uint16_t dstCID = ((uint16_t)data[5] << 8) | data[4];
  uint16_t result = ((uint16_t)data[9] << 8) | data[8];

  if (result != 0x0000) {
    return;
  }

  l2cap_connection_t connection;
  connection.ch = ch;
  connection.remoteCID = dstCID;
  if (l2cap_add_connection(connection) < 0) {
    return;
  }

  uint8_t pos = 0;
  l2capPayload[pos++] = 0x04;  // CONFIGURATION REQUEST
  l2capPayload[pos++] = 0x02;  // Identifier
  l2capPayload[pos++] = 0x08;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = (uint8_t)(dstCID & 0xFF);
  l2capPayload[pos++] = (uint8_t)(dstCID >> 8);
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = 0x01;
  l2capPayload[pos++] = 0x02;
  l2capPayload[pos++] = 0x40;
  l2capPayload[pos++] = 0x00;

  send_acl_l2cap_packet(ch, 0x0001, l2capPayload, pos);
}

void l2cap_signaling_handle_configuration_request(uint16_t ch, uint8_t* data, uint16_t len) {
  if (len < 12) {
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
  if (l2cap_get_remote_cid(ch, &remoteCID) != 0) {
    return;
  }

  uint8_t pos = 0;
  l2capPayload[pos++] = 0x05;  // CONFIGURATION RESPONSE
  l2capPayload[pos++] = identifier;
  l2capPayload[pos++] = 0x0A;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = (uint8_t)(remoteCID & 0xFF);
  l2capPayload[pos++] = (uint8_t)(remoteCID >> 8);
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = 0x00;
  l2capPayload[pos++] = 0x01;
  l2capPayload[pos++] = 0x02;
  l2capPayload[pos++] = (uint8_t)(mtu & 0xFF);
  l2capPayload[pos++] = (uint8_t)(mtu >> 8);

  send_acl_l2cap_packet(ch, 0x0001, l2capPayload, pos);
}

void l2cap_signaling_handle_configuration_response(uint8_t* data, uint16_t len) {
  (void)data;
  (void)len;
}
