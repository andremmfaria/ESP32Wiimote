#ifndef WIIMOTE_STATE_H
#define WIIMOTE_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        bool wiimoteConnected;
        bool nunchukConnected;
        uint8_t batteryLevel;
        bool useAccelerometer;
    } WiimoteState;

    void wiimote_state_init(WiimoteState *state);
    void wiimote_state_reset(WiimoteState *state);

    void wiimote_state_set_connected(WiimoteState *state, bool connected);
    bool wiimote_state_is_connected(const WiimoteState *state);

    void wiimote_state_set_nunchuk_connected(WiimoteState *state, bool connected);
    bool wiimote_state_is_nunchuk_connected(const WiimoteState *state);

    void wiimote_state_set_battery_level(WiimoteState *state, uint8_t level);
    uint8_t wiimote_state_get_battery_level(const WiimoteState *state);

    void wiimote_state_set_use_accelerometer(WiimoteState *state, bool use);
    bool wiimote_state_get_use_accelerometer(const WiimoteState *state);

#ifdef __cplusplus
}
#endif

#endif
