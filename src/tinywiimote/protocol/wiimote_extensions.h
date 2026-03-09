#ifndef WIIMOTE_EXTENSIONS_H
#define WIIMOTE_EXTENSIONS_H

#include <stdint.h>
#include "wiimote_state.h"
#include "../l2cap/l2cap_connection.h"
#include "../l2cap/l2cap_packets.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int controllerReportState;
        WiimoteState *state;
        const L2capConnectionTable *connections;
        L2capPacketSender *sender;
    } WiimoteExtensions;

    void wiimote_extensions_init(WiimoteExtensions *extensions, WiimoteState *state,
                                 const L2capConnectionTable *connections,
                                 L2capPacketSender *sender);
    void wiimote_extensions_handle_report(WiimoteExtensions *extensions, uint16_t ch,
                                          uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
