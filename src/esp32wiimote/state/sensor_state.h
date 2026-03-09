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
typedef struct {
    uint8_t xAxis;
    uint8_t yAxis;
    uint8_t zAxis;
} AccelState;

/**
 * Nunchuk State - Analog stick and accelerometer
 */
typedef struct {
    uint8_t xStick;
    uint8_t yStick;
    uint8_t xAxis;
    uint8_t yAxis;
    uint8_t zAxis;
} NunchukState;

/**
 * Sensor State Manager - Tracks accelerometer and nunchuk sensor data
 */
class SensorStateManager {
public:
    SensorStateManager(int nunchukStickThreshold = 1);
    
    /**
     * Update accelerometer state
     */
    void updateAccel(AccelState state);
    
    /**
     * Update nunchuk state
     */
    void updateNunchuk(NunchukState state);
    
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
    AccelState getAccel(void) const;
    
    /**
     * Get current nunchuk state
     */
    NunchukState getNunchuk(void) const;
    
    /**
     * Get previous accelerometer state
     */
    AccelState getPreviousAccel(void) const;
    
    /**
     * Get previous nunchuk state
     */
    NunchukState getPreviousNunchuk(void) const;
    
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
    AccelState _currentAccel;
    AccelState _previousAccel;
    
    NunchukState _currentNunchuk;
    NunchukState _previousNunchuk;
    
    int _nunchukStickThreshold;
};

#endif // __SENSOR_STATE_H__
