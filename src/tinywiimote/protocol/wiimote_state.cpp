#include "wiimote_state.h"

#include "../../utils/serial_logging.h"

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
    // Wiimote sends raw battery value 0-0xD0 (0-208)
    // Convert to percentage 0-100
    // Full battery is typically 0xC8 (200) but max is 0xD0 (208)
    uint8_t percentage = (level > 208) ? 100 : (level * 100) / 208;
    batteryLevel = percentage;
    LOG_DEBUG("WiimoteState: Battery: raw=0x%02x (%d) -> %d%%\n", level, level, percentage);
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
