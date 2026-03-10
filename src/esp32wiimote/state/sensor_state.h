// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __SENSOR_STATE_H__
#define __SENSOR_STATE_H__

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
    void resetAccel(void);

    /**
     * Reset nunchuk state to zero
     */
    void resetNunchuk(void);

    /**
     * Get current accelerometer state
     */
    struct AccelState getAccel(void) const;

    /**
     * Get current nunchuk state
     */
    struct NunchukState getNunchuk(void) const;

    /**
     * Get previous accelerometer state
     */
    struct AccelState getPreviousAccel(void) const;

    /**
     * Get previous nunchuk state
     */
    struct NunchukState getPreviousNunchuk(void) const;

    /**
     * Check if accelerometer state changed
     */
    bool accelHasChanged(void) const;

    /**
     * Check if nunchuk stick changed (based on threshold)
     */
    bool nunchukStickHasChanged(void) const;

    /**
     * Reset change flags and store current as previous
     */
    void resetChangeFlags(void);

   private:
    struct AccelState _currentAccel;
    struct AccelState _previousAccel;

    struct NunchukState _currentNunchuk;
    struct NunchukState _previousNunchuk;

    int _nunchukStickThreshold;
};

#endif  // __SENSOR_STATE_H__
