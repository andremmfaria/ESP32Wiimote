#ifndef WIIMOTE_EXTENSIONS_H
#define WIIMOTE_EXTENSIONS_H

#include "../l2cap/l2cap_connection.h"
#include "../l2cap/l2cap_packets.h"
#include "wiimote_state.h"

#include <stdint.h>

class WiimoteExtensions {
   public:
    WiimoteExtensions();

    void init(WiimoteState *state,
              const L2capConnectionTable *connectionTable,
              L2capPacketSender *sender);
    void handleReport(uint16_t ch, uint8_t *data, uint16_t len);

   private:
    int controllerReportState = 0;
    WiimoteState *state = nullptr;
    const L2capConnectionTable *connections = nullptr;
    L2capPacketSender *sender = nullptr;
};

#endif
