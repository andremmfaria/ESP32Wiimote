#ifndef WIIMOTE_STATE_H
#define WIIMOTE_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void wiimote_state_init(void);
void wiimote_state_reset(void);

void wiimote_state_set_connected(bool connected);
bool wiimote_state_is_connected(void);

void wiimote_state_set_nunchuk_connected(bool connected);
bool wiimote_state_is_nunchuk_connected(void);

void wiimote_state_set_battery_level(uint8_t level);
uint8_t wiimote_state_get_battery_level(void);

void wiimote_state_set_use_accelerometer(bool use);
bool wiimote_state_get_use_accelerometer(void);

#ifdef __cplusplus
}
#endif

#endif
