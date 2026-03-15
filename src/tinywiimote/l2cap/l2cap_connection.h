// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_L2_CAP_CONNECTION_H
#define ESP32_WIIMOTE_L2_CAP_CONNECTION_H

#include <stdint.h>

class L2capConnection {
   public:
    struct Endpoint {
        uint16_t channelHandle;
        uint16_t remoteCid;
    };

    L2capConnection();
    explicit L2capConnection(const Endpoint &endpoint);

    uint16_t ch;
    uint16_t remoteCID;
};

#define L2CAP_CONNECTION_LIST_SIZE 8

class L2capConnectionTable {
   public:
    L2capConnectionTable();

    void clear();
    int findConnection(uint16_t ch) const;
    int addConnection(const L2capConnection &connection);
    int getRemoteCid(uint16_t ch, uint16_t *remoteCID) const;
    int getFirstConnectionHandle(uint16_t *ch) const;

   private:
    L2capConnection list_[L2CAP_CONNECTION_LIST_SIZE];
    int size_;
};

#endif  // ESP32_WIIMOTE_L2_CAP_CONNECTION_H
