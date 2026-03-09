#include "wiimote_state.h"

static bool wiimoteConnected = false;
static bool nunchukConnected = false;
static uint8_t batteryLevel = 0;
static bool useAccelerometer = true;

void wiimote_state_init(void) {
  wiimoteConnected = false;
  nunchukConnected = false;
  batteryLevel = 0;
  useAccelerometer = true;
}

void wiimote_state_reset(void) {
  wiimoteConnected = false;
  nunchukConnected = false;
  batteryLevel = 0;
}

void wiimote_state_set_connected(bool connected) {
  wiimoteConnected = connected;
}

bool wiimote_state_is_connected(void) {
  return wiimoteConnected;
}

void wiimote_state_set_nunchuk_connected(bool connected) {
  nunchukConnected = connected;
}

bool wiimote_state_is_nunchuk_connected(void) {
  return nunchukConnected;
}

void wiimote_state_set_battery_level(uint8_t level) {
  batteryLevel = level;
}

uint8_t wiimote_state_get_battery_level(void) {
  return batteryLevel;
}

void wiimote_state_set_use_accelerometer(bool use) {
  useAccelerometer = use;
}

bool wiimote_state_get_use_accelerometer(void) {
  return useAccelerometer;
}
