#include "l2cap_signaling.h"

void l2cap_signaling_init(L2capSignaling* signaling, L2capConnectionTable* connections,
                          L2capPacketSender* sender) {
  if (signaling == 0) {
    return;
  }
  signaling->connections = connections;
  signaling->sender = sender;
}

void l2cap_signaling_send_connection_request(L2capSignaling* signaling, uint16_t ch, uint16_t psm, uint16_t cid) {
  if (signaling == 0) {
    return;
  }

  uint8_t pos = 0;
  signaling->payload[pos++] = 0x02;  // CONNECTION REQUEST
  signaling->payload[pos++] = 0x01;  // Identifier
  signaling->payload[pos++] = 0x04;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = (uint8_t)(psm & 0xFF);
  signaling->payload[pos++] = (uint8_t)(psm >> 8);
  signaling->payload[pos++] = (uint8_t)(cid & 0xFF);
  signaling->payload[pos++] = (uint8_t)(cid >> 8);

  send_acl_l2cap_packet(signaling->sender, ch, 0x0001, signaling->payload, pos);
}

void l2cap_signaling_handle_connection_response(L2capSignaling* signaling, uint16_t ch, uint8_t* data, uint16_t len) {
  if (len < 12) {
    return;
  }

  if (signaling == 0 || signaling->connections == 0 || signaling->sender == 0) {
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
  if (l2cap_add_connection(signaling->connections, connection) < 0) {
    return;
  }

  uint8_t pos = 0;
  signaling->payload[pos++] = 0x04;  // CONFIGURATION REQUEST
  signaling->payload[pos++] = 0x02;  // Identifier
  signaling->payload[pos++] = 0x08;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = (uint8_t)(dstCID & 0xFF);
  signaling->payload[pos++] = (uint8_t)(dstCID >> 8);
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = 0x01;
  signaling->payload[pos++] = 0x02;
  signaling->payload[pos++] = 0x40;
  signaling->payload[pos++] = 0x00;

  send_acl_l2cap_packet(signaling->sender, ch, 0x0001, signaling->payload, pos);
}

void l2cap_signaling_handle_configuration_request(L2capSignaling* signaling, uint16_t ch, uint8_t* data, uint16_t len) {
  if (len < 12) {
    return;
  }

  if (signaling == 0 || signaling->connections == 0 || signaling->sender == 0) {
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
  if (l2cap_get_remote_cid(signaling->connections, ch, &remoteCID) != 0) {
    return;
  }

  uint8_t pos = 0;
  signaling->payload[pos++] = 0x05;  // CONFIGURATION RESPONSE
  signaling->payload[pos++] = identifier;
  signaling->payload[pos++] = 0x0A;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = (uint8_t)(remoteCID & 0xFF);
  signaling->payload[pos++] = (uint8_t)(remoteCID >> 8);
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = 0x00;
  signaling->payload[pos++] = 0x01;
  signaling->payload[pos++] = 0x02;
  signaling->payload[pos++] = (uint8_t)(mtu & 0xFF);
  signaling->payload[pos++] = (uint8_t)(mtu >> 8);

  send_acl_l2cap_packet(signaling->sender, ch, 0x0001, signaling->payload, pos);
}

void l2cap_signaling_handle_configuration_response(L2capSignaling* signaling, uint8_t* data, uint16_t len) {
  (void)signaling;
  (void)data;
  (void)len;
}
