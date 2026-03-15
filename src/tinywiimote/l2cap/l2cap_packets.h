// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_L2_CAP_PACKETS_H
#define ESP32_WIIMOTE_L2_CAP_PACKETS_H

#include <stddef.h>
#include <stdint.h>

typedef void (*L2capRawSendFunc)(uint8_t *data, size_t len);

struct AclPacketControl {
    uint8_t packetBoundaryFlag;
    uint8_t broadcastFlag;
};

class L2capPacketSender {
   public:
    L2capPacketSender();

    void setSendCallback(L2capRawSendFunc callback);
    void sendAclL2capPacket(uint16_t ch, uint16_t remoteCID, uint8_t *payload, uint16_t payloadLen);

   private:
    L2capRawSendFunc sendCallback_{nullptr};
    uint8_t tmpQueueData_[256];
};

uint16_t makeL2capPacket(uint8_t *buf, uint16_t channelID, const uint8_t *data, uint16_t len);
uint16_t makeAclL2capPacket(uint8_t *buf,
                            uint16_t ch,
                            const AclPacketControl &control,
                            uint16_t channelID,
                            uint8_t *data,
                            uint8_t len);

#endif  // ESP32_WIIMOTE_L2_CAP_PACKETS_H
