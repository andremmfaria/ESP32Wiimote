#include "wiimote_state.h"

WiimoteState::WiimoteState() {
  reset();
  useAccelerometer = true;
}

void WiimoteState::reset() {
  wiimoteConnected = false;
  nunchukConnected = false;
  batteryLevel = 0;
}

void WiimoteState::setConnected(bool connected) {
  wiimoteConnected = connected;
}

bool WiimoteState::isConnected() const {
  return wiimoteConnected;
}

void WiimoteState::setNunchukConnected(bool connected) {
  nunchukConnected = connected;
}

bool WiimoteState::isNunchukConnected() const {
  return nunchukConnected;
}

void WiimoteState::setBatteryLevel(uint8_t level) {
  batteryLevel = level;
}

uint8_t WiimoteState::getBatteryLevel() const {
  return batteryLevel;
}

void WiimoteState::setUseAccelerometer(bool use) {
  useAccelerometer = use;
}

bool WiimoteState::getUseAccelerometer() const {
  return useAccelerometer;
}
