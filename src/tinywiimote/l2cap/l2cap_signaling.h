#ifndef L2CAP_SIGNALING_H
#define L2CAP_SIGNALING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void l2cap_signaling_send_connection_request(uint16_t ch, uint16_t psm, uint16_t cid);
void l2cap_signaling_handle_connection_response(uint16_t ch, uint8_t* data, uint16_t len);
void l2cap_signaling_handle_configuration_request(uint16_t ch, uint8_t* data, uint16_t len);
void l2cap_signaling_handle_configuration_response(uint8_t* data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif
