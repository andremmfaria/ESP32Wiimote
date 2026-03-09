// Copyright (c) 2020 Daiki Yasuda
//
// This is licensed under
// - Creative Commons Attribution-NonCommercial 3.0 Unported
// - https://creativecommons.org/licenses/by-nc/3.0/
// - Or see LICENSE.md

#ifndef __BUTTON_STATE_H__
#define __BUTTON_STATE_H__

#include <stdint.h>

/**
 * Button State Type - Represents Wiimote button states
 */
typedef enum {
    BUTTON_Z          = 0x00020000, // nunchuk
    BUTTON_C          = 0x00010000, // nunchuk
    BUTTON_PLUS       = 0x00001000,
    BUTTON_UP         = 0x00000800, // vertical orientation
    BUTTON_DOWN       = 0x00000400,
    BUTTON_RIGHT      = 0x00000200,
    BUTTON_LEFT       = 0x00000100,
    BUTTON_HOME       = 0x00000080,
    BUTTON_MINUS      = 0x00000010,
    BUTTON_A          = 0x00000008,
    BUTTON_B          = 0x00000004,
    BUTTON_ONE        = 0x00000002,
    BUTTON_TWO        = 0x00000001,
    NO_BUTTON         = 0x00000000
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
    ButtonState getCurrent(void) const;
    
    /**
     * Get previous button state
     */
    ButtonState getPrevious(void) const;
    
    /**
     * Check if button state changed
     */
    bool hasChanged(void) const;
    
    /**
     * Reset state change flag and store current as old
     */
    void resetChangeFlag(void);

private:
    ButtonState _current;
    ButtonState _previous;
};

#endif // __BUTTON_STATE_H__
