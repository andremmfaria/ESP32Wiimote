// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __DATA_PARSER_H__
#define __DATA_PARSER_H__

#include "TinyWiimote.h"
#include "state/button_state.h"
#include "state/sensor_state.h"

#include <stdint.h>

/**
 * Filter flags for ignoring certain data types
 */
enum {
    FILTER_NONE = 0x0000,
    FILTER_BUTTON = 0x0001,
    FILTER_NUNCHUK_STICK = 0x0004,
    FILTER_ACCEL = 0x0008,
};

/**
 * Wiimote Data Parser
 * Parses incoming Wiimote sensor data and detects changes
 */
class WiimoteDataParser {
   public:
    WiimoteDataParser(ButtonStateManager* buttonState, SensorStateManager* sensorState);

    /**
     * Parse incoming Wiimote data and update state
     * @return 1 if any monitored data changed, 0 otherwise
     */
    int parseData(void);

    /**
     * Set filter flags
     */
    void setFilter(int filter);

    /**
     * Get current filter flags
     */
    int getFilter(void) const;

   private:
    ButtonStateManager* _buttonState;
    SensorStateManager* _sensorState;
    int _filter;

    /**
     * Parse button data from Wiimote report
     */
    void parseButtonData(const TinyWiimoteData& data, int& buttonChanged);

    /**
     * Parse accelerometer data from Wiimote report
     */
    void parseAccelData(const TinyWiimoteData& data, int& accelChanged);

    /**
     * Parse nunchuk/extension data from Wiimote report
     */
    void parseNunchukData(const TinyWiimoteData& data,
                          int& nunchukStickChanged,
                          int& accelChanged,
                          int& buttonChanged);
};

#endif  // __DATA_PARSER_H__
