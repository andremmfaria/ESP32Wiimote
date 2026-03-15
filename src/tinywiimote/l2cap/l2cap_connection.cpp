// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#include "l2cap_connection.h"

#include "../../utils/serial_logging.h"

L2capConnection::L2capConnection() : ch(0), remoteCID(0) {}

L2capConnection::L2capConnection(const Endpoint &endpoint)
    : ch(endpoint.channelHandle), remoteCID(endpoint.remoteCid) {}

L2capConnectionTable::L2capConnectionTable() : size_(0) {}

void L2capConnectionTable::clear() {
    size_ = 0;
}

int L2capConnectionTable::findConnection(uint16_t ch) const {
    for (int i = 0; i < size_; i++) {
        if (list_[i].ch == ch) {
            return i;
        }
    }
    return -1;
}

int L2capConnectionTable::addConnection(const L2capConnection &connection) {
    if (size_ >= L2CAP_CONNECTION_LIST_SIZE) {
        LOG_WARN("L2CAP: Connection table full, cannot add connection\n");
        return -1;
    }

    LOG_DEBUG("L2CAP: Adding connection: ch=0x%04x remoteCID=0x%04x\n", connection.ch,
              connection.remoteCID);
    list_[size_++] = connection;
    return size_;
}

int L2capConnectionTable::getRemoteCid(uint16_t ch, uint16_t *remoteCID) const {
    const int kIdx = findConnection(ch);
    if (kIdx < 0 || remoteCID == nullptr) {
        return -1;
    }

    *remoteCID = list_[kIdx].remoteCID;
    return 0;
}

int L2capConnectionTable::getFirstConnectionHandle(uint16_t *ch) const {
    if (size_ == 0 || ch == nullptr) {
        return -1;
    }

    *ch = list_[0].ch;
    return 0;
}
