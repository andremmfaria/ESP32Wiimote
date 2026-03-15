// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef ESP32_WIIMOTE_BUTTON_STATE_H
#define ESP32_WIIMOTE_BUTTON_STATE_H

#include <stdint.h>

/**
 * Button State Type - Represents Wiimote button states
 */
typedef enum {
    ButtonZ = 0x00020000,   // nunchuk
    ButtonC = 0x00010000,   // nunchuk
    ButtonPlus = 0x00001000,
    ButtonUp = 0x00000800,  // vertical orientation
    ButtonDown = 0x00000400,
    ButtonRight = 0x00000200,
    ButtonLeft = 0x00000100,
    ButtonHome = 0x00000080,
    ButtonMinus = 0x00000010,
    ButtonA = 0x00000008,
    ButtonB = 0x00000004,
    ButtonOne = 0x00000002,
    ButtonTwo = 0x00000001,
    NoButton = 0x00000000
} ButtonState;

/**
 * Button State Manager - Tracks button state changes
 */
class ButtonStateManager {
   public:
    ButtonStateManager();

    /**
     * Update button state from raw Wiimote data
     * @param currentState New button state
     */
    void update(ButtonState currentState);

    /**
     * Get current button state
     */
    ButtonState getCurrent() const;

    /**
     * Get previous button state
     */
    ButtonState getPrevious() const;

    /**
     * Check if button state changed
     */
    bool hasChanged() const;

    /**
     * Reset state change flag and store current as old
     */
    void resetChangeFlag();

   private:
    ButtonState current_;
    ButtonState previous_;
};

#endif  // ESP32_WIIMOTE_BUTTON_STATE_H
