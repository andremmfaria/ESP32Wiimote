#ifndef L2CAP_SIGNALING_H
#define L2CAP_SIGNALING_H

#include "l2cap_connection.h"
#include "l2cap_packets.h"

#include <stddef.h>
#include <stdint.h>

class L2capSignaling {
   public:
    L2capSignaling();

    void init(L2capConnectionTable *connectionTable, L2capPacketSender *sender);
    void sendConnectionRequest(uint16_t ch, uint16_t psm, uint16_t cid);
    void handleConnectionResponse(uint16_t ch, uint8_t *data, uint16_t len);
    void handleConfigurationRequest(uint16_t ch, uint8_t *data, uint16_t len);
    static void handleConfigurationResponse(const uint8_t *data, uint16_t len);

   private:
    L2capConnectionTable *connections = nullptr;
    L2capPacketSender *sender = nullptr;
    uint8_t payload[64];
};

#endif
