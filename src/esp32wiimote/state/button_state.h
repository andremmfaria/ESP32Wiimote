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
enum class ButtonState : uint32_t {
    Z = 0x00020000,   // nunchuk
    C = 0x00010000,   // nunchuk
    Plus = 0x00001000,
    Up = 0x00000800,  // vertical orientation
    Down = 0x00000400,
    Right = 0x00000200,
    Left = 0x00000100,
    Home = 0x00000080,
    Minus = 0x00000010,
    A = 0x00000008,
    B = 0x00000004,
    One = 0x00000002,
    Two = 0x00000001,
    None = 0x00000000,
};

static constexpr ButtonState kButtonZ = ButtonState::Z;
static constexpr ButtonState kButtonC = ButtonState::C;
static constexpr ButtonState kButtonPlus = ButtonState::Plus;
static constexpr ButtonState kButtonUp = ButtonState::Up;
static constexpr ButtonState kButtonDown = ButtonState::Down;
static constexpr ButtonState kButtonRight = ButtonState::Right;
static constexpr ButtonState kButtonLeft = ButtonState::Left;
static constexpr ButtonState kButtonHome = ButtonState::Home;
static constexpr ButtonState kButtonMinus = ButtonState::Minus;
static constexpr ButtonState kButtonA = ButtonState::A;
static constexpr ButtonState kButtonB = ButtonState::B;
static constexpr ButtonState kButtonOne = ButtonState::One;
static constexpr ButtonState kButtonTwo = ButtonState::Two;
static constexpr ButtonState kNoButton = ButtonState::None;

static inline ButtonState buttonStateFromRaw(uint32_t raw) {
    return static_cast<ButtonState>(raw);
}

static inline ButtonState buttonStateOr(ButtonState lhs, ButtonState rhs) {
    return static_cast<ButtonState>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

static inline bool buttonStateHas(ButtonState state, ButtonState flag) {
    return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0U;
}

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
