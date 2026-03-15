// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_SENSOR_STATE_H
#define ESP32_WIIMOTE_SENSOR_STATE_H

#include <stdint.h>

/**
 * Accelerometer State
 */
struct AccelState {
    uint8_t xAxis;
    uint8_t yAxis;
    uint8_t zAxis;
};

/**
 * Nunchuk State - Analog stick and accelerometer
 */
struct NunchukState {
    uint8_t xStick;
    uint8_t yStick;
    uint8_t xAxis;
    uint8_t yAxis;
    uint8_t zAxis;
};

/**
 * Sensor State Manager - Tracks accelerometer and nunchuk sensor data
 */
class SensorStateManager {
   public:
    SensorStateManager(int nunchukStickThreshold = 1);

    /**
     * Update accelerometer state
     */
    void updateAccel(struct AccelState state);

    /**
     * Update nunchuk state
     */
    void updateNunchuk(struct NunchukState state);

    /**
     * Reset accelerometer state to zero
     */
    void resetAccel();

    /**
     * Reset nunchuk state to zero
     */
    void resetNunchuk();

    /**
     * Get current accelerometer state
     */
    struct AccelState getAccel() const;

    /**
     * Get current nunchuk state
     */
    struct NunchukState getNunchuk() const;

    /**
     * Get previous accelerometer state
     */
    struct AccelState getPreviousAccel() const;

    /**
     * Get previous nunchuk state
     */
    struct NunchukState getPreviousNunchuk() const;

    /**
     * Check if accelerometer state changed
     */
    bool accelHasChanged() const;

    /**
     * Check if nunchuk stick changed (based on threshold)
     */
    bool nunchukStickHasChanged() const;

    /**
     * Reset change flags and store current as previous
     */
    void resetChangeFlags();

   private:
    struct AccelState currentAccel_;
    struct AccelState previousAccel_;

    struct NunchukState currentNunchuk_;
    struct NunchukState previousNunchuk_;

    int nunchukStickThreshold_;
};

#endif  // ESP32_WIIMOTE_SENSOR_STATE_H
