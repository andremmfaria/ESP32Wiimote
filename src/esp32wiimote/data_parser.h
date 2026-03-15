// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_DATA_PARSER_H
#define ESP32_WIIMOTE_DATA_PARSER_H

#include "TinyWiimote.h"
#include "state/button_state.h"
#include "state/sensor_state.h"

#include <stdint.h>

/**
 * Filter flags for ignoring certain data types
 */
enum class WiimoteDataFilter : uint16_t {
    None = 0x0000,
    Button = 0x0001,
    NunchukStick = 0x0004,
    Accel = 0x0008,
};

static constexpr int kFilterNone = static_cast<int>(WiimoteDataFilter::None);
static constexpr int kFilterButton = static_cast<int>(WiimoteDataFilter::Button);
static constexpr int kFilterNunchukStick = static_cast<int>(WiimoteDataFilter::NunchukStick);
static constexpr int kFilterAccel = static_cast<int>(WiimoteDataFilter::Accel);

static inline bool hasFilterFlag(int filter, WiimoteDataFilter flag) {
    return (filter & static_cast<int>(flag)) != 0;
}

/**
 * Wiimote Data Parser
 * Parses incoming Wiimote sensor data and detects changes
 */
class WiimoteDataParser {
   public:
    WiimoteDataParser(ButtonStateManager *buttonState, SensorStateManager *sensorState);

    /**
     * Parse incoming Wiimote data and update state
     * @return 1 if any monitored data changed, 0 otherwise
     */
    int parseData();

    /**
     * Set filter flags
     */
    void setFilter(int filter);

    /**
     * Get current filter flags
     */
    int getFilter() const;

   private:
    struct ChangeFlags {
        int buttonChanged;
        int accelChanged;
        int nunchukStickChanged;
    };

    ButtonStateManager *buttonState_;
    SensorStateManager *sensorState_;
    int filter_;

    /**
     * Parse button data from Wiimote report
     */
    void parseButtonData(const TinyWiimoteData &data, int &buttonChanged);

    /**
     * Parse accelerometer data from Wiimote report
     */
    void parseAccelData(const TinyWiimoteData &data, int &accelChanged);

    /**
     * Parse nunchuk/extension data from Wiimote report
     */
    void parseNunchukData(const TinyWiimoteData &data, ChangeFlags &flags);
};

#endif  // ESP32_WIIMOTE_DATA_PARSER_H
