// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __L2CAP_PACKETS_H__
#define __L2CAP_PACKETS_H__

#include <stddef.h>
#include <stdint.h>

typedef void (*L2capRawSendFunc)(uint8_t *data, size_t len);

class L2capPacketSender {
   public:
    L2capPacketSender();

    void setSendCallback(L2capRawSendFunc callback);
    void sendAclL2capPacket(uint16_t ch, uint16_t remoteCID, uint8_t *payload, uint16_t payloadLen);

   private:
    L2capRawSendFunc sendCallback;
    uint8_t tmpQueueData[256];
};

uint16_t make_l2cap_packet(uint8_t *buf, uint16_t channelID, const uint8_t *data, uint16_t len);
uint16_t make_acl_l2cap_packet(uint8_t *buf,
                               uint16_t ch,
                               uint8_t pbf,
                               uint8_t bf,
                               uint16_t channelID,
                               uint8_t *data,
                               uint8_t len);

#endif  // __L2CAP_PACKETS_H__
