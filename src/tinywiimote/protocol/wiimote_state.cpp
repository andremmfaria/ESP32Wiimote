#include "wiimote_state.h"

#include "../../utils/serial_logging.h"

WiimoteState::WiimoteState() {
    reset();
    useAccelerometer_ = true;
}

void WiimoteState::reset() {
    wiimoteConnected_ = false;
    nunchukConnected_ = false;
    batteryLevel_ = 0;
}

void WiimoteState::setConnected(bool connected) {
    wiimoteConnected_ = connected;
}

bool WiimoteState::isConnected() const {
    return wiimoteConnected_;
}

void WiimoteState::setNunchukConnected(bool connected) {
    nunchukConnected_ = connected;
}

bool WiimoteState::isNunchukConnected() const {
    return nunchukConnected_;
}

void WiimoteState::setBatteryLevel(uint8_t level) {
    // Wiimote sends raw battery value 0-0xD0 (0-208)
    // Convert to percentage 0-100
    // Full battery is typically 0xC8 (200) but max is 0xD0 (208)
    uint8_t percentage = (level > 208) ? 100 : (level * 100) / 208;
    batteryLevel_ = percentage;
    wiimoteLogDebug("WiimoteState: Battery: raw=0x%02x (%d) -> %d%%\n", level, level, percentage);
}

uint8_t WiimoteState::getBatteryLevel() const {
    return batteryLevel_;
}

void WiimoteState::setUseAccelerometer(bool use) {
    useAccelerometer_ = use;
}

bool WiimoteState::getUseAccelerometer() const {
    return useAccelerometer_;
}
