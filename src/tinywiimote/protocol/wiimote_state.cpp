#include "wiimote_state.h"

void wiimote_state_init(WiimoteState* state) {
  if (state == 0) {
    return;
  }
  state->wiimoteConnected = false;
  state->nunchukConnected = false;
  state->batteryLevel = 0;
  state->useAccelerometer = true;
}

void wiimote_state_reset(WiimoteState* state) {
  if (state == 0) {
    return;
  }
  state->wiimoteConnected = false;
  state->nunchukConnected = false;
  state->batteryLevel = 0;
}

void wiimote_state_set_connected(WiimoteState* state, bool connected) {
  if (state == 0) {
    return;
  }
  state->wiimoteConnected = connected;
}

bool wiimote_state_is_connected(const WiimoteState* state) {
  return state != 0 && state->wiimoteConnected;
}

void wiimote_state_set_nunchuk_connected(WiimoteState* state, bool connected) {
  if (state == 0) {
    return;
  }
  state->nunchukConnected = connected;
}

bool wiimote_state_is_nunchuk_connected(const WiimoteState* state) {
  return state != 0 && state->nunchukConnected;
}

void wiimote_state_set_battery_level(WiimoteState* state, uint8_t level) {
  if (state == 0) {
    return;
  }
  state->batteryLevel = level;
}

uint8_t wiimote_state_get_battery_level(const WiimoteState* state) {
  if (state == 0) {
    return 0;
  }
  return state->batteryLevel;
}

void wiimote_state_set_use_accelerometer(WiimoteState* state, bool use) {
  if (state == 0) {
    return;
  }
  state->useAccelerometer = use;
}

bool wiimote_state_get_use_accelerometer(const WiimoteState* state) {
  return state != 0 && state->useAccelerometer;
}
