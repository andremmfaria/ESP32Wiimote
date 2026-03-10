// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __L2CAP_CONNECTION_H__
#define __L2CAP_CONNECTION_H__

#include <stdint.h>

class L2capConnection {
   public:
    L2capConnection();
    L2capConnection(uint16_t channelHandle, uint16_t remoteCid);

    uint16_t ch;
    uint16_t remoteCID;
};

#define L2CAP_CONNECTION_LIST_SIZE 8

class L2capConnectionTable {
   public:
    L2capConnectionTable();

    void clear();
    int findConnection(uint16_t ch) const;
    int addConnection(const L2capConnection& connection);
    int getRemoteCid(uint16_t ch, uint16_t* remoteCID) const;
    int getFirstConnectionHandle(uint16_t* ch) const;

   private:
    L2capConnection list[L2CAP_CONNECTION_LIST_SIZE];
    int size;
};

#endif  // __L2CAP_CONNECTION_H__
