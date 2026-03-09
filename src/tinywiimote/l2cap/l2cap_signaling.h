#ifndef L2CAP_SIGNALING_H
#define L2CAP_SIGNALING_H

#include <stddef.h>
#include <stdint.h>

#include "l2cap_connection.h"
#include "l2cap_packets.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        L2capConnectionTable *connections;
        L2capPacketSender *sender;
        uint8_t payload[64];
    } L2capSignaling;

    void l2cap_signaling_init(L2capSignaling *signaling, L2capConnectionTable *connections,
                              L2capPacketSender *sender);
    void l2cap_signaling_send_connection_request(L2capSignaling *signaling, uint16_t ch, uint16_t psm, uint16_t cid);
    void l2cap_signaling_handle_connection_response(L2capSignaling *signaling, uint16_t ch, uint8_t *data, uint16_t len);
    void l2cap_signaling_handle_configuration_request(L2capSignaling *signaling, uint16_t ch, uint8_t *data, uint16_t len);
    void l2cap_signaling_handle_configuration_response(L2capSignaling *signaling, uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
