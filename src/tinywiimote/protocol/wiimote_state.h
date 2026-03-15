#ifndef WIIMOTE_STATE_H
#define WIIMOTE_STATE_H

#include <stdint.h>

class WiimoteState {
   public:
    WiimoteState();

    void reset();

    void setConnected(bool connected);
    bool isConnected() const;

    void setNunchukConnected(bool connected);
    bool isNunchukConnected() const;

    void setBatteryLevel(uint8_t level);
    uint8_t getBatteryLevel() const;

    void setUseAccelerometer(bool use);
    bool getUseAccelerometer() const;

   private:
    bool wiimoteConnected_;
    bool nunchukConnected_;
    uint8_t batteryLevel_;
    bool useAccelerometer_;
};

#endif
