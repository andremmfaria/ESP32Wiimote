// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "l2cap_connection.h"
#include "../../utils/serial_logging.h"

L2capConnection::L2capConnection() : ch(0), remoteCID(0) {}

L2capConnection::L2capConnection(uint16_t channelHandle, uint16_t remoteCid)
    : ch(channelHandle), remoteCID(remoteCid) {}

L2capConnectionTable::L2capConnectionTable() : size(0) {}

void L2capConnectionTable::clear() {
  size = 0;
}

int L2capConnectionTable::findConnection(uint16_t ch) const {
  for (int i = 0; i < size; i++) {
    if (list[i].ch == ch) {
      return i;
    }
  }
  return -1;
}

int L2capConnectionTable::addConnection(const L2capConnection& connection) {
  if (size >= L2CAP_CONNECTION_LIST_SIZE) {
    LOG_WARN("L2CAP: Connection table full, cannot add connection\n");
    return -1;
  }

  LOG_DEBUG("L2CAP: Adding connection: ch=0x%04x remoteCID=0x%04x\n", connection.ch, connection.remoteCID);
  list[size++] = connection;
  return size;
}

int L2capConnectionTable::getRemoteCid(uint16_t ch, uint16_t* remoteCID) const {
  const int idx = findConnection(ch);
  if (idx < 0 || remoteCID == 0) {
    return -1;
  }

  *remoteCID = list[idx].remoteCID;
  return 0;
}
